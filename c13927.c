static int recv_stream(struct kiocb *iocb, struct socket *sock,
		       struct msghdr *m, size_t buf_len, int flags)
{
	struct sock *sk = sock->sk;
	struct tipc_port *tport = tipc_sk_port(sk);
	struct sk_buff *buf;
	struct tipc_msg *msg;
	long timeout;
	unsigned int sz;
	int sz_to_copy, target, needed;
	int sz_copied = 0;
	u32 err;
	int res = 0;

	/* Catch invalid receive attempts */
	if (unlikely(!buf_len))
		return -EINVAL;

	lock_sock(sk);

	if (unlikely((sock->state == SS_UNCONNECTED) ||
		     (sock->state == SS_CONNECTING))) {
		res = -ENOTCONN;
		goto exit;
	}

	target = sock_rcvlowat(sk, flags & MSG_WAITALL, buf_len);
	timeout = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

restart:
	/* Look for a message in receive queue; wait if necessary */
	while (skb_queue_empty(&sk->sk_receive_queue)) {
		if (sock->state == SS_DISCONNECTING) {
			res = -ENOTCONN;
			goto exit;
		}
		if (timeout <= 0L) {
			res = timeout ? timeout : -EWOULDBLOCK;
			goto exit;
		}
		release_sock(sk);
		timeout = wait_event_interruptible_timeout(*sk_sleep(sk),
							   tipc_rx_ready(sock),
							   timeout);
		lock_sock(sk);
	}

	/* Look at first message in receive queue */
	buf = skb_peek(&sk->sk_receive_queue);
	msg = buf_msg(buf);
	sz = msg_data_sz(msg);
	err = msg_errcode(msg);

	/* Discard an empty non-errored message & try again */
	if ((!sz) && (!err)) {
		advance_rx_queue(sk);
		goto restart;
	}

	/* Optionally capture sender's address & ancillary data of first msg */
	if (sz_copied == 0) {
		set_orig_addr(m, msg);
		res = anc_data_recv(m, msg, tport);
		if (res)
			goto exit;
	}

	/* Capture message data (if valid) & compute return value (always) */
	if (!err) {
		u32 offset = (u32)(unsigned long)(TIPC_SKB_CB(buf)->handle);

		sz -= offset;
		needed = (buf_len - sz_copied);
		sz_to_copy = (sz <= needed) ? sz : needed;

		res = skb_copy_datagram_iovec(buf, msg_hdr_sz(msg) + offset,
					      m->msg_iov, sz_to_copy);
		if (res)
			goto exit;

		sz_copied += sz_to_copy;

		if (sz_to_copy < sz) {
			if (!(flags & MSG_PEEK))
				TIPC_SKB_CB(buf)->handle =
				(void *)(unsigned long)(offset + sz_to_copy);
			goto exit;
		}
	} else {
		if (sz_copied != 0)
			goto exit; /* can't add error msg to valid data */

		if ((err == TIPC_CONN_SHUTDOWN) || m->msg_control)
			res = 0;
		else
			res = -ECONNRESET;
	}

	/* Consume received message (optional) */
	if (likely(!(flags & MSG_PEEK))) {
		if (unlikely(++tport->conn_unacked >= TIPC_FLOW_CONTROL_WIN))
			tipc_acknowledge(tport->ref, tport->conn_unacked);
		advance_rx_queue(sk);
	}

	/* Loop around if more data is required */
	if ((sz_copied < buf_len) &&	/* didn't get all requested data */
	    (!skb_queue_empty(&sk->sk_receive_queue) ||
	    (sz_copied < target)) &&	/* and more is ready or required */
	    (!(flags & MSG_PEEK)) &&	/* and aren't just peeking at data */
	    (!err))			/* and haven't reached a FIN */
		goto restart;

exit:
	release_sock(sk);
	return sz_copied ? sz_copied : res;
}