static int sctp_v6_xmit(struct sk_buff *skb, struct sctp_transport *transport)
{
	struct sock *sk = skb->sk;
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct flowi6 fl6;

	memset(&fl6, 0, sizeof(fl6));

	fl6.flowi6_proto = sk->sk_protocol;

	/* Fill in the dest address from the route entry passed with the skb
	 * and the source address from the transport.
	 */
	fl6.daddr = transport->ipaddr.v6.sin6_addr;
	fl6.saddr = transport->saddr.v6.sin6_addr;

	fl6.flowlabel = np->flow_label;
	IP6_ECN_flow_xmit(sk, fl6.flowlabel);
	if (ipv6_addr_type(&fl6.saddr) & IPV6_ADDR_LINKLOCAL)
		fl6.flowi6_oif = transport->saddr.v6.sin6_scope_id;
	else
		fl6.flowi6_oif = sk->sk_bound_dev_if;

	if (np->opt && np->opt->srcrt) {
		struct rt0_hdr *rt0 = (struct rt0_hdr *) np->opt->srcrt;
		fl6.daddr = *rt0->addr;
	}

	pr_debug("%s: skb:%p, len:%d, src:%pI6 dst:%pI6\n", __func__, skb,
		 skb->len, &fl6.saddr, &fl6.daddr);

	SCTP_INC_STATS(sock_net(sk), SCTP_MIB_OUTSCTPPACKS);

	if (!(transport->param_flags & SPP_PMTUD_ENABLE))
		skb->local_df = 1;

	return ip6_xmit(sk, skb, &fl6, np->opt, np->tclass);
}