int ping_recvmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
		 size_t len, int noblock, int flags, int *addr_len)
{
	struct inet_sock *isk = inet_sk(sk);
	int family = sk->sk_family;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	struct sk_buff *skb;
	int copied, err;

	pr_debug("ping_recvmsg(sk=%p,sk->num=%u)\n", isk, isk->inet_num);

	err = -EOPNOTSUPP;
	if (flags & MSG_OOB)
		goto out;

	if (addr_len) {
		if (family == AF_INET)
			*addr_len = sizeof(*sin);
		else if (family == AF_INET6 && addr_len)
			*addr_len = sizeof(*sin6);
	}

	if (flags & MSG_ERRQUEUE) {
		if (family == AF_INET) {
			return ip_recv_error(sk, msg, len);
#if IS_ENABLED(CONFIG_IPV6)
		} else if (family == AF_INET6) {
			return pingv6_ops.ipv6_recv_error(sk, msg, len);
#endif
		}
	}

	skb = skb_recv_datagram(sk, flags, noblock, &err);
	if (!skb)
		goto out;

	copied = skb->len;
	if (copied > len) {
		msg->msg_flags |= MSG_TRUNC;
		copied = len;
	}

	/* Don't bother checking the checksum */
	err = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (err)
		goto done;

	sock_recv_timestamp(msg, sk, skb);

	/* Copy the address and add cmsg data. */
	if (family == AF_INET) {
		sin = (struct sockaddr_in *) msg->msg_name;
		sin->sin_family = AF_INET;
		sin->sin_port = 0 /* skb->h.uh->source */;
		sin->sin_addr.s_addr = ip_hdr(skb)->saddr;
		memset(sin->sin_zero, 0, sizeof(sin->sin_zero));

		if (isk->cmsg_flags)
			ip_cmsg_recv(msg, skb);

#if IS_ENABLED(CONFIG_IPV6)
	} else if (family == AF_INET6) {
		struct ipv6_pinfo *np = inet6_sk(sk);
		struct ipv6hdr *ip6 = ipv6_hdr(skb);
		sin6 = (struct sockaddr_in6 *) msg->msg_name;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		sin6->sin6_addr = ip6->saddr;

		sin6->sin6_flowinfo = 0;
		if (np->sndflow)
			sin6->sin6_flowinfo = ip6_flowinfo(ip6);

		sin6->sin6_scope_id = ipv6_iface_scope_id(&sin6->sin6_addr,
							  IP6CB(skb)->iif);

		if (inet6_sk(sk)->rxopt.all)
			pingv6_ops.ip6_datagram_recv_ctl(sk, msg, skb);
#endif
	} else {
		BUG();
	}

	err = copied;

done:
	skb_free_datagram(sk, skb);
out:
	pr_debug("ping_recvmsg -> %d\n", err);
	return err;
}