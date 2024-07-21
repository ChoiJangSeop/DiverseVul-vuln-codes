int ipv6_recv_rxpmtu(struct sock *sk, struct msghdr *msg, int len)
{
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sk_buff *skb;
	struct sockaddr_in6 *sin;
	struct ip6_mtuinfo mtu_info;
	int err;
	int copied;

	err = -EAGAIN;
	skb = xchg(&np->rxpmtu, NULL);
	if (skb == NULL)
		goto out;

	copied = skb->len;
	if (copied > len) {
		msg->msg_flags |= MSG_TRUNC;
		copied = len;
	}
	err = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (err)
		goto out_free_skb;

	sock_recv_timestamp(msg, sk, skb);

	memcpy(&mtu_info, IP6CBMTU(skb), sizeof(mtu_info));

	sin = (struct sockaddr_in6 *)msg->msg_name;
	if (sin) {
		sin->sin6_family = AF_INET6;
		sin->sin6_flowinfo = 0;
		sin->sin6_port = 0;
		sin->sin6_scope_id = mtu_info.ip6m_addr.sin6_scope_id;
		sin->sin6_addr = mtu_info.ip6m_addr.sin6_addr;
	}

	put_cmsg(msg, SOL_IPV6, IPV6_PATHMTU, sizeof(mtu_info), &mtu_info);

	err = copied;

out_free_skb:
	kfree_skb(skb);
out:
	return err;
}