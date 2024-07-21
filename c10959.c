static int unix_dgram_recvmsg(struct socket *sock, struct msghdr *msg,
			      size_t size, int flags)
{
	struct scm_cookie scm;
	struct sock *sk = sock->sk;
	struct unix_sock *u = unix_sk(sk);
	struct sk_buff *skb, *last;
	long timeo;
	int skip;
	int err;

	err = -EOPNOTSUPP;
	if (flags&MSG_OOB)
		goto out;

	timeo = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

	do {
		mutex_lock(&u->iolock);

		skip = sk_peek_offset(sk, flags);
		skb = __skb_try_recv_datagram(sk, &sk->sk_receive_queue, flags,
					      &skip, &err, &last);
		if (skb) {
			if (!(flags & MSG_PEEK))
				scm_stat_del(sk, skb);
			break;
		}

		mutex_unlock(&u->iolock);

		if (err != -EAGAIN)
			break;
	} while (timeo &&
		 !__skb_wait_for_more_packets(sk, &sk->sk_receive_queue,
					      &err, &timeo, last));

	if (!skb) { /* implies iolock unlocked */
		unix_state_lock(sk);
		/* Signal EOF on disconnected non-blocking SEQPACKET socket. */
		if (sk->sk_type == SOCK_SEQPACKET && err == -EAGAIN &&
		    (sk->sk_shutdown & RCV_SHUTDOWN))
			err = 0;
		unix_state_unlock(sk);
		goto out;
	}

	if (wq_has_sleeper(&u->peer_wait))
		wake_up_interruptible_sync_poll(&u->peer_wait,
						EPOLLOUT | EPOLLWRNORM |
						EPOLLWRBAND);

	if (msg->msg_name)
		unix_copy_addr(msg, skb->sk);

	if (size > skb->len - skip)
		size = skb->len - skip;
	else if (size < skb->len - skip)
		msg->msg_flags |= MSG_TRUNC;

	err = skb_copy_datagram_msg(skb, skip, msg, size);
	if (err)
		goto out_free;

	if (sock_flag(sk, SOCK_RCVTSTAMP))
		__sock_recv_timestamp(msg, sk, skb);

	memset(&scm, 0, sizeof(scm));

	scm_set_cred(&scm, UNIXCB(skb).pid, UNIXCB(skb).uid, UNIXCB(skb).gid);
	unix_set_secdata(&scm, skb);

	if (!(flags & MSG_PEEK)) {
		if (UNIXCB(skb).fp)
			unix_detach_fds(&scm, skb);

		sk_peek_offset_bwd(sk, skb->len);
	} else {
		/* It is questionable: on PEEK we could:
		   - do not return fds - good, but too simple 8)
		   - return fds, and do not return them on read (old strategy,
		     apparently wrong)
		   - clone fds (I chose it for now, it is the most universal
		     solution)

		   POSIX 1003.1g does not actually define this clearly
		   at all. POSIX 1003.1g doesn't define a lot of things
		   clearly however!

		*/

		sk_peek_offset_fwd(sk, size);

		if (UNIXCB(skb).fp)
			scm.fp = scm_fp_dup(UNIXCB(skb).fp);
	}
	err = (flags & MSG_TRUNC) ? skb->len - skip : size;

	scm_recv(sock, msg, &scm, flags);

out_free:
	skb_free_datagram(sk, skb);
	mutex_unlock(&u->iolock);
out:
	return err;
}