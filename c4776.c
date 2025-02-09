static void ip6gre_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
		u8 type, u8 code, int offset, __be32 info)
{
	const struct ipv6hdr *ipv6h = (const struct ipv6hdr *)skb->data;
	__be16 *p = (__be16 *)(skb->data + offset);
	int grehlen = offset + 4;
	struct ip6_tnl *t;
	__be16 flags;

	flags = p[0];
	if (flags&(GRE_CSUM|GRE_KEY|GRE_SEQ|GRE_ROUTING|GRE_VERSION)) {
		if (flags&(GRE_VERSION|GRE_ROUTING))
			return;
		if (flags&GRE_KEY) {
			grehlen += 4;
			if (flags&GRE_CSUM)
				grehlen += 4;
		}
	}

	/* If only 8 bytes returned, keyed message will be dropped here */
	if (!pskb_may_pull(skb, grehlen))
		return;
	ipv6h = (const struct ipv6hdr *)skb->data;
	p = (__be16 *)(skb->data + offset);

	t = ip6gre_tunnel_lookup(skb->dev, &ipv6h->daddr, &ipv6h->saddr,
				flags & GRE_KEY ?
				*(((__be32 *)p) + (grehlen / 4) - 1) : 0,
				p[1]);
	if (!t)
		return;

	switch (type) {
		__u32 teli;
		struct ipv6_tlv_tnl_enc_lim *tel;
		__u32 mtu;
	case ICMPV6_DEST_UNREACH:
		net_dbg_ratelimited("%s: Path to destination invalid or inactive!\n",
				    t->parms.name);
		break;
	case ICMPV6_TIME_EXCEED:
		if (code == ICMPV6_EXC_HOPLIMIT) {
			net_dbg_ratelimited("%s: Too small hop limit or routing loop in tunnel!\n",
					    t->parms.name);
		}
		break;
	case ICMPV6_PARAMPROB:
		teli = 0;
		if (code == ICMPV6_HDR_FIELD)
			teli = ip6_tnl_parse_tlv_enc_lim(skb, skb->data);

		if (teli && teli == be32_to_cpu(info) - 2) {
			tel = (struct ipv6_tlv_tnl_enc_lim *) &skb->data[teli];
			if (tel->encap_limit == 0) {
				net_dbg_ratelimited("%s: Too small encapsulation limit or routing loop in tunnel!\n",
						    t->parms.name);
			}
		} else {
			net_dbg_ratelimited("%s: Recipient unable to parse tunneled packet!\n",
					    t->parms.name);
		}
		break;
	case ICMPV6_PKT_TOOBIG:
		mtu = be32_to_cpu(info) - offset;
		if (mtu < IPV6_MIN_MTU)
			mtu = IPV6_MIN_MTU;
		t->dev->mtu = mtu;
		break;
	}

	if (time_before(jiffies, t->err_time + IP6TUNNEL_ERR_TIMEO))
		t->err_count++;
	else
		t->err_count = 1;
	t->err_time = jiffies;
}