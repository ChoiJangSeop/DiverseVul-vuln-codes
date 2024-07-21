vsock_stream_recvmsg(struct socket *sock, struct msghdr *msg, size_t len,
		     int flags)
{
	struct sock *sk;
	struct vsock_sock *vsk;
	const struct vsock_transport *transport;
	int err;
	size_t target;
	ssize_t copied;
	long timeout;
	struct vsock_transport_recv_notify_data recv_data;

	DEFINE_WAIT(wait);

	sk = sock->sk;
	vsk = vsock_sk(sk);
	transport = vsk->transport;
	err = 0;

	lock_sock(sk);

	if (!transport || sk->sk_state != TCP_ESTABLISHED) {
		/* Recvmsg is supposed to return 0 if a peer performs an
		 * orderly shutdown. Differentiate between that case and when a
		 * peer has not connected or a local shutdown occured with the
		 * SOCK_DONE flag.
		 */
		if (sock_flag(sk, SOCK_DONE))
			err = 0;
		else
			err = -ENOTCONN;

		goto out;
	}

	if (flags & MSG_OOB) {
		err = -EOPNOTSUPP;
		goto out;
	}

	/* We don't check peer_shutdown flag here since peer may actually shut
	 * down, but there can be data in the queue that a local socket can
	 * receive.
	 */
	if (sk->sk_shutdown & RCV_SHUTDOWN) {
		err = 0;
		goto out;
	}

	/* It is valid on Linux to pass in a zero-length receive buffer.  This
	 * is not an error.  We may as well bail out now.
	 */
	if (!len) {
		err = 0;
		goto out;
	}

	/* We must not copy less than target bytes into the user's buffer
	 * before returning successfully, so we wait for the consume queue to
	 * have that much data to consume before dequeueing.  Note that this
	 * makes it impossible to handle cases where target is greater than the
	 * queue size.
	 */
	target = sock_rcvlowat(sk, flags & MSG_WAITALL, len);
	if (target >= transport->stream_rcvhiwat(vsk)) {
		err = -ENOMEM;
		goto out;
	}
	timeout = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);
	copied = 0;

	err = transport->notify_recv_init(vsk, target, &recv_data);
	if (err < 0)
		goto out;


	while (1) {
		s64 ready;

		prepare_to_wait(sk_sleep(sk), &wait, TASK_INTERRUPTIBLE);
		ready = vsock_stream_has_data(vsk);

		if (ready == 0) {
			if (sk->sk_err != 0 ||
			    (sk->sk_shutdown & RCV_SHUTDOWN) ||
			    (vsk->peer_shutdown & SEND_SHUTDOWN)) {
				finish_wait(sk_sleep(sk), &wait);
				break;
			}
			/* Don't wait for non-blocking sockets. */
			if (timeout == 0) {
				err = -EAGAIN;
				finish_wait(sk_sleep(sk), &wait);
				break;
			}

			err = transport->notify_recv_pre_block(
					vsk, target, &recv_data);
			if (err < 0) {
				finish_wait(sk_sleep(sk), &wait);
				break;
			}
			release_sock(sk);
			timeout = schedule_timeout(timeout);
			lock_sock(sk);

			if (signal_pending(current)) {
				err = sock_intr_errno(timeout);
				finish_wait(sk_sleep(sk), &wait);
				break;
			} else if (timeout == 0) {
				err = -EAGAIN;
				finish_wait(sk_sleep(sk), &wait);
				break;
			}
		} else {
			ssize_t read;

			finish_wait(sk_sleep(sk), &wait);

			if (ready < 0) {
				/* Invalid queue pair content. XXX This should
				* be changed to a connection reset in a later
				* change.
				*/

				err = -ENOMEM;
				goto out;
			}

			err = transport->notify_recv_pre_dequeue(
					vsk, target, &recv_data);
			if (err < 0)
				break;

			read = transport->stream_dequeue(
					vsk, msg,
					len - copied, flags);
			if (read < 0) {
				err = -ENOMEM;
				break;
			}

			copied += read;

			err = transport->notify_recv_post_dequeue(
					vsk, target, read,
					!(flags & MSG_PEEK), &recv_data);
			if (err < 0)
				goto out;

			if (read >= target || flags & MSG_PEEK)
				break;

			target -= read;
		}
	}

	if (sk->sk_err)
		err = -sk->sk_err;
	else if (sk->sk_shutdown & RCV_SHUTDOWN)
		err = 0;

	if (copied > 0)
		err = copied;

out:
	release_sock(sk);
	return err;
}