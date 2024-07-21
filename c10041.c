static int vsock_stream_sendmsg(struct socket *sock, struct msghdr *msg,
				size_t len)
{
	struct sock *sk;
	struct vsock_sock *vsk;
	const struct vsock_transport *transport;
	ssize_t total_written;
	long timeout;
	int err;
	struct vsock_transport_send_notify_data send_data;
	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	sk = sock->sk;
	vsk = vsock_sk(sk);
	transport = vsk->transport;
	total_written = 0;
	err = 0;

	if (msg->msg_flags & MSG_OOB)
		return -EOPNOTSUPP;

	lock_sock(sk);

	/* Callers should not provide a destination with stream sockets. */
	if (msg->msg_namelen) {
		err = sk->sk_state == TCP_ESTABLISHED ? -EISCONN : -EOPNOTSUPP;
		goto out;
	}

	/* Send data only if both sides are not shutdown in the direction. */
	if (sk->sk_shutdown & SEND_SHUTDOWN ||
	    vsk->peer_shutdown & RCV_SHUTDOWN) {
		err = -EPIPE;
		goto out;
	}

	if (!transport || sk->sk_state != TCP_ESTABLISHED ||
	    !vsock_addr_bound(&vsk->local_addr)) {
		err = -ENOTCONN;
		goto out;
	}

	if (!vsock_addr_bound(&vsk->remote_addr)) {
		err = -EDESTADDRREQ;
		goto out;
	}

	/* Wait for room in the produce queue to enqueue our user's data. */
	timeout = sock_sndtimeo(sk, msg->msg_flags & MSG_DONTWAIT);

	err = transport->notify_send_init(vsk, &send_data);
	if (err < 0)
		goto out;

	while (total_written < len) {
		ssize_t written;

		add_wait_queue(sk_sleep(sk), &wait);
		while (vsock_stream_has_space(vsk) == 0 &&
		       sk->sk_err == 0 &&
		       !(sk->sk_shutdown & SEND_SHUTDOWN) &&
		       !(vsk->peer_shutdown & RCV_SHUTDOWN)) {

			/* Don't wait for non-blocking sockets. */
			if (timeout == 0) {
				err = -EAGAIN;
				remove_wait_queue(sk_sleep(sk), &wait);
				goto out_err;
			}

			err = transport->notify_send_pre_block(vsk, &send_data);
			if (err < 0) {
				remove_wait_queue(sk_sleep(sk), &wait);
				goto out_err;
			}

			release_sock(sk);
			timeout = wait_woken(&wait, TASK_INTERRUPTIBLE, timeout);
			lock_sock(sk);
			if (signal_pending(current)) {
				err = sock_intr_errno(timeout);
				remove_wait_queue(sk_sleep(sk), &wait);
				goto out_err;
			} else if (timeout == 0) {
				err = -EAGAIN;
				remove_wait_queue(sk_sleep(sk), &wait);
				goto out_err;
			}
		}
		remove_wait_queue(sk_sleep(sk), &wait);

		/* These checks occur both as part of and after the loop
		 * conditional since we need to check before and after
		 * sleeping.
		 */
		if (sk->sk_err) {
			err = -sk->sk_err;
			goto out_err;
		} else if ((sk->sk_shutdown & SEND_SHUTDOWN) ||
			   (vsk->peer_shutdown & RCV_SHUTDOWN)) {
			err = -EPIPE;
			goto out_err;
		}

		err = transport->notify_send_pre_enqueue(vsk, &send_data);
		if (err < 0)
			goto out_err;

		/* Note that enqueue will only write as many bytes as are free
		 * in the produce queue, so we don't need to ensure len is
		 * smaller than the queue size.  It is the caller's
		 * responsibility to check how many bytes we were able to send.
		 */

		written = transport->stream_enqueue(
				vsk, msg,
				len - total_written);
		if (written < 0) {
			err = -ENOMEM;
			goto out_err;
		}

		total_written += written;

		err = transport->notify_send_post_enqueue(
				vsk, written, &send_data);
		if (err < 0)
			goto out_err;

	}

out_err:
	if (total_written > 0)
		err = total_written;
out:
	release_sock(sk);
	return err;
}