static int tipc_udp_xmit(struct net *net, struct sk_buff *skb,
			 struct udp_bearer *ub, struct udp_media_addr *src,
			 struct udp_media_addr *dst, struct dst_cache *cache)
{
	struct dst_entry *ndst = dst_cache_get(cache);
	int ttl, err = 0;

	if (dst->proto == htons(ETH_P_IP)) {
		struct rtable *rt = (struct rtable *)ndst;

		if (!rt) {
			struct flowi4 fl = {
				.daddr = dst->ipv4.s_addr,
				.saddr = src->ipv4.s_addr,
				.flowi4_mark = skb->mark,
				.flowi4_proto = IPPROTO_UDP
			};
			rt = ip_route_output_key(net, &fl);
			if (IS_ERR(rt)) {
				err = PTR_ERR(rt);
				goto tx_error;
			}
			dst_cache_set_ip4(cache, &rt->dst, fl.saddr);
		}

		ttl = ip4_dst_hoplimit(&rt->dst);
		udp_tunnel_xmit_skb(rt, ub->ubsock->sk, skb, src->ipv4.s_addr,
				    dst->ipv4.s_addr, 0, ttl, 0, src->port,
				    dst->port, false, true);
#if IS_ENABLED(CONFIG_IPV6)
	} else {
		if (!ndst) {
			struct flowi6 fl6 = {
				.flowi6_oif = ub->ifindex,
				.daddr = dst->ipv6,
				.saddr = src->ipv6,
				.flowi6_proto = IPPROTO_UDP
			};
			err = ipv6_stub->ipv6_dst_lookup(net, ub->ubsock->sk,
							 &ndst, &fl6);
			if (err)
				goto tx_error;
			dst_cache_set_ip6(cache, ndst, &fl6.saddr);
		}
		ttl = ip6_dst_hoplimit(ndst);
		err = udp_tunnel6_xmit_skb(ndst, ub->ubsock->sk, skb, NULL,
					   &src->ipv6, &dst->ipv6, 0, ttl, 0,
					   src->port, dst->port, false);
#endif
	}
	return err;

tx_error:
	kfree_skb(skb);
	return err;
}