int vcc_recvmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg,
		size_t size, int flags)
{
	struct sock *sk = sock->sk;
	struct atm_vcc *vcc;
	struct sk_buff *skb;
	int copied, error = -EINVAL;

	msg->msg_namelen = 0;

	if (sock->state != SS_CONNECTED)
		return -ENOTCONN;

	/* only handle MSG_DONTWAIT and MSG_PEEK */
	if (flags & ~(MSG_DONTWAIT | MSG_PEEK))
		return -EOPNOTSUPP;

	vcc = ATM_SD(sock);
	if (test_bit(ATM_VF_RELEASED, &vcc->flags) ||
	    test_bit(ATM_VF_CLOSE, &vcc->flags) ||
	    !test_bit(ATM_VF_READY, &vcc->flags))
		return 0;

	skb = skb_recv_datagram(sk, flags, flags & MSG_DONTWAIT, &error);
	if (!skb)
		return error;

	copied = skb->len;
	if (copied > size) {
		copied = size;
		msg->msg_flags |= MSG_TRUNC;
	}

	error = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (error)
		return error;
	sock_recv_ts_and_drops(msg, sk, skb);

	if (!(flags & MSG_PEEK)) {
		pr_debug("%d -= %d\n", atomic_read(&sk->sk_rmem_alloc),
			 skb->truesize);
		atm_return(vcc, skb->truesize);
	}

	skb_free_datagram(sk, skb);
	return copied;
}