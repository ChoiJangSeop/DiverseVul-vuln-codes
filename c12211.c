int bt_sock_recvmsg(struct kiocb *iocb, struct socket *sock,
				struct msghdr *msg, size_t len, int flags)
{
	int noblock = flags & MSG_DONTWAIT;
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	size_t copied;
	int err;

	BT_DBG("sock %p sk %p len %zu", sock, sk, len);

	if (flags & (MSG_OOB))
		return -EOPNOTSUPP;

	skb = skb_recv_datagram(sk, flags, noblock, &err);
	if (!skb) {
		if (sk->sk_shutdown & RCV_SHUTDOWN) {
			msg->msg_namelen = 0;
			return 0;
		}
		return err;
	}

	copied = skb->len;
	if (len < copied) {
		msg->msg_flags |= MSG_TRUNC;
		copied = len;
	}

	skb_reset_transport_header(skb);
	err = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (err == 0) {
		sock_recv_ts_and_drops(msg, sk, skb);

		if (bt_sk(sk)->skb_msg_name)
			bt_sk(sk)->skb_msg_name(skb, msg->msg_name,
						&msg->msg_namelen);
		else
			msg->msg_namelen = 0;
	}

	skb_free_datagram(sk, skb);

	return err ? : copied;
}