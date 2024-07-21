static int geneve6_xmit_skb(struct sk_buff *skb, struct net_device *dev,
			    struct geneve_dev *geneve,
			    const struct ip_tunnel_info *info)
{
	bool xnet = !net_eq(geneve->net, dev_net(geneve->dev));
	struct geneve_sock *gs6 = rcu_dereference(geneve->sock6);
	const struct ip_tunnel_key *key = &info->key;
	struct dst_entry *dst = NULL;
	struct flowi6 fl6;
	__u8 prio, ttl;
	__be16 sport;
	int err;

	dst = geneve_get_v6_dst(skb, dev, gs6, &fl6, info);
	if (IS_ERR(dst))
		return PTR_ERR(dst);

	err = skb_tunnel_check_pmtu(skb, dst,
				    GENEVE_IPV6_HLEN + info->options_len,
				    netif_is_any_bridge_port(dev));
	if (err < 0) {
		dst_release(dst);
		return err;
	} else if (err) {
		struct ip_tunnel_info *info = skb_tunnel_info(skb);

		if (info) {
			info->key.u.ipv6.dst = fl6.saddr;
			info->key.u.ipv6.src = fl6.daddr;
		}

		if (!pskb_may_pull(skb, ETH_HLEN)) {
			dst_release(dst);
			return -EINVAL;
		}

		skb->protocol = eth_type_trans(skb, geneve->dev);
		netif_rx(skb);
		dst_release(dst);
		return -EMSGSIZE;
	}

	sport = udp_flow_src_port(geneve->net, skb, 1, USHRT_MAX, true);
	if (geneve->cfg.collect_md) {
		prio = ip_tunnel_ecn_encap(key->tos, ip_hdr(skb), skb);
		ttl = key->ttl;
	} else {
		prio = ip_tunnel_ecn_encap(ip6_tclass(fl6.flowlabel),
					   ip_hdr(skb), skb);
		if (geneve->cfg.ttl_inherit)
			ttl = ip_tunnel_get_ttl(ip_hdr(skb), skb);
		else
			ttl = key->ttl;
		ttl = ttl ? : ip6_dst_hoplimit(dst);
	}
	err = geneve_build_skb(dst, skb, info, xnet, sizeof(struct ipv6hdr));
	if (unlikely(err))
		return err;

	udp_tunnel6_xmit_skb(dst, gs6->sock->sk, skb, dev,
			     &fl6.saddr, &fl6.daddr, prio, ttl,
			     info->key.label, sport, geneve->cfg.info.key.tp_dst,
			     !(info->key.tun_flags & TUNNEL_CSUM));
	return 0;
}