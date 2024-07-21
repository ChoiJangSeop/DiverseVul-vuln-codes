int ipv6_recv_error(struct sock *sk, struct msghdr *msg, int len)
{
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sock_exterr_skb *serr;
	struct sk_buff *skb, *skb2;
	struct sockaddr_in6 *sin;
	struct {
		struct sock_extended_err ee;
		struct sockaddr_in6	 offender;
	} errhdr;
	int err;
	int copied;

	err = -EAGAIN;
	skb = skb_dequeue(&sk->sk_error_queue);
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

	serr = SKB_EXT_ERR(skb);

	sin = (struct sockaddr_in6 *)msg->msg_name;
	if (sin) {
		const unsigned char *nh = skb_network_header(skb);
		sin->sin6_family = AF_INET6;
		sin->sin6_flowinfo = 0;
		sin->sin6_port = serr->port;
		if (skb->protocol == htons(ETH_P_IPV6)) {
			const struct ipv6hdr *ip6h = container_of((struct in6_addr *)(nh + serr->addr_offset),
								  struct ipv6hdr, daddr);
			sin->sin6_addr = ip6h->daddr;
			if (np->sndflow)
				sin->sin6_flowinfo = ip6_flowinfo(ip6h);
			sin->sin6_scope_id =
				ipv6_iface_scope_id(&sin->sin6_addr,
						    IP6CB(skb)->iif);
		} else {
			ipv6_addr_set_v4mapped(*(__be32 *)(nh + serr->addr_offset),
					       &sin->sin6_addr);
			sin->sin6_scope_id = 0;
		}
	}

	memcpy(&errhdr.ee, &serr->ee, sizeof(struct sock_extended_err));
	sin = &errhdr.offender;
	sin->sin6_family = AF_UNSPEC;
	if (serr->ee.ee_origin != SO_EE_ORIGIN_LOCAL) {
		sin->sin6_family = AF_INET6;
		sin->sin6_flowinfo = 0;
		if (skb->protocol == htons(ETH_P_IPV6)) {
			sin->sin6_addr = ipv6_hdr(skb)->saddr;
			if (np->rxopt.all)
				ip6_datagram_recv_ctl(sk, msg, skb);
			sin->sin6_scope_id =
				ipv6_iface_scope_id(&sin->sin6_addr,
						    IP6CB(skb)->iif);
		} else {
			struct inet_sock *inet = inet_sk(sk);

			ipv6_addr_set_v4mapped(ip_hdr(skb)->saddr,
					       &sin->sin6_addr);
			sin->sin6_scope_id = 0;
			if (inet->cmsg_flags)
				ip_cmsg_recv(msg, skb);
		}
	}

	put_cmsg(msg, SOL_IPV6, IPV6_RECVERR, sizeof(errhdr), &errhdr);

	/* Now we could try to dump offended packet options */

	msg->msg_flags |= MSG_ERRQUEUE;
	err = copied;

	/* Reset and regenerate socket error */
	spin_lock_bh(&sk->sk_error_queue.lock);
	sk->sk_err = 0;
	if ((skb2 = skb_peek(&sk->sk_error_queue)) != NULL) {
		sk->sk_err = SKB_EXT_ERR(skb2)->ee.ee_errno;
		spin_unlock_bh(&sk->sk_error_queue.lock);
		sk->sk_error_report(sk);
	} else {
		spin_unlock_bh(&sk->sk_error_queue.lock);
	}

out_free_skb:
	kfree_skb(skb);
out:
	return err;
}