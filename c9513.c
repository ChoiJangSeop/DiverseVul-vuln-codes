static int geneve_fill_metadata_dst(struct net_device *dev, struct sk_buff *skb)
{
	struct ip_tunnel_info *info = skb_tunnel_info(skb);
	struct geneve_dev *geneve = netdev_priv(dev);

	if (ip_tunnel_info_af(info) == AF_INET) {
		struct rtable *rt;
		struct flowi4 fl4;
		struct geneve_sock *gs4 = rcu_dereference(geneve->sock4);

		rt = geneve_get_v4_rt(skb, dev, gs4, &fl4, info);
		if (IS_ERR(rt))
			return PTR_ERR(rt);

		ip_rt_put(rt);
		info->key.u.ipv4.src = fl4.saddr;
#if IS_ENABLED(CONFIG_IPV6)
	} else if (ip_tunnel_info_af(info) == AF_INET6) {
		struct dst_entry *dst;
		struct flowi6 fl6;
		struct geneve_sock *gs6 = rcu_dereference(geneve->sock6);

		dst = geneve_get_v6_dst(skb, dev, gs6, &fl6, info);
		if (IS_ERR(dst))
			return PTR_ERR(dst);

		dst_release(dst);
		info->key.u.ipv6.src = fl6.saddr;
#endif
	} else {
		return -EINVAL;
	}

	info->key.tp_src = udp_flow_src_port(geneve->net, skb,
					     1, USHRT_MAX, true);
	info->key.tp_dst = geneve->cfg.info.key.tp_dst;
	return 0;
}