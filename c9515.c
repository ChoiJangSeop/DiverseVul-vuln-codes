static int geneve_xmit_skb(struct sk_buff *skb, struct net_device *dev,
			   struct geneve_dev *geneve,
			   const struct ip_tunnel_info *info)
{
	bool xnet = !net_eq(geneve->net, dev_net(geneve->dev));
	struct geneve_sock *gs4 = rcu_dereference(geneve->sock4);
	const struct ip_tunnel_key *key = &info->key;
	struct rtable *rt;
	struct flowi4 fl4;
	__u8 tos, ttl;
	__be16 df = 0;
	__be16 sport;
	int err;

	rt = geneve_get_v4_rt(skb, dev, gs4, &fl4, info);
	if (IS_ERR(rt))
		return PTR_ERR(rt);

	err = skb_tunnel_check_pmtu(skb, &rt->dst,
				    GENEVE_IPV4_HLEN + info->options_len,
				    netif_is_any_bridge_port(dev));
	if (err < 0) {
		dst_release(&rt->dst);
		return err;
	} else if (err) {
		struct ip_tunnel_info *info;

		info = skb_tunnel_info(skb);
		if (info) {
			info->key.u.ipv4.dst = fl4.saddr;
			info->key.u.ipv4.src = fl4.daddr;
		}

		if (!pskb_may_pull(skb, ETH_HLEN)) {
			dst_release(&rt->dst);
			return -EINVAL;
		}

		skb->protocol = eth_type_trans(skb, geneve->dev);
		netif_rx(skb);
		dst_release(&rt->dst);
		return -EMSGSIZE;
	}

	sport = udp_flow_src_port(geneve->net, skb, 1, USHRT_MAX, true);
	if (geneve->cfg.collect_md) {
		tos = ip_tunnel_ecn_encap(key->tos, ip_hdr(skb), skb);
		ttl = key->ttl;

		df = key->tun_flags & TUNNEL_DONT_FRAGMENT ? htons(IP_DF) : 0;
	} else {
		tos = ip_tunnel_ecn_encap(fl4.flowi4_tos, ip_hdr(skb), skb);
		if (geneve->cfg.ttl_inherit)
			ttl = ip_tunnel_get_ttl(ip_hdr(skb), skb);
		else
			ttl = key->ttl;
		ttl = ttl ? : ip4_dst_hoplimit(&rt->dst);

		if (geneve->cfg.df == GENEVE_DF_SET) {
			df = htons(IP_DF);
		} else if (geneve->cfg.df == GENEVE_DF_INHERIT) {
			struct ethhdr *eth = eth_hdr(skb);

			if (ntohs(eth->h_proto) == ETH_P_IPV6) {
				df = htons(IP_DF);
			} else if (ntohs(eth->h_proto) == ETH_P_IP) {
				struct iphdr *iph = ip_hdr(skb);

				if (iph->frag_off & htons(IP_DF))
					df = htons(IP_DF);
			}
		}
	}

	err = geneve_build_skb(&rt->dst, skb, info, xnet, sizeof(struct iphdr));
	if (unlikely(err))
		return err;

	udp_tunnel_xmit_skb(rt, gs4->sock->sk, skb, fl4.saddr, fl4.daddr,
			    tos, ttl, df, sport, geneve->cfg.info.key.tp_dst,
			    !net_eq(geneve->net, dev_net(geneve->dev)),
			    !(info->key.tun_flags & TUNNEL_CSUM));
	return 0;
}