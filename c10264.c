static int qrtr_recvmsg(struct socket *sock, struct msghdr *msg,
			size_t size, int flags)
{
	DECLARE_SOCKADDR(struct sockaddr_qrtr *, addr, msg->msg_name);
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	struct qrtr_cb *cb;
	int copied, rc;

	lock_sock(sk);

	if (sock_flag(sk, SOCK_ZAPPED)) {
		release_sock(sk);
		return -EADDRNOTAVAIL;
	}

	skb = skb_recv_datagram(sk, flags & ~MSG_DONTWAIT,
				flags & MSG_DONTWAIT, &rc);
	if (!skb) {
		release_sock(sk);
		return rc;
	}
	cb = (struct qrtr_cb *)skb->cb;

	copied = skb->len;
	if (copied > size) {
		copied = size;
		msg->msg_flags |= MSG_TRUNC;
	}

	rc = skb_copy_datagram_msg(skb, 0, msg, copied);
	if (rc < 0)
		goto out;
	rc = copied;

	if (addr) {
		addr->sq_family = AF_QIPCRTR;
		addr->sq_node = cb->src_node;
		addr->sq_port = cb->src_port;
		msg->msg_namelen = sizeof(*addr);
	}

out:
	if (cb->confirm_rx)
		qrtr_send_resume_tx(cb);

	skb_free_datagram(sk, skb);
	release_sock(sk);

	return rc;
}