static int mlx5e_route_lookup_ipv6(struct mlx5e_priv *priv,
				   struct net_device *mirred_dev,
				   struct net_device **out_dev,
				   struct net_device **route_dev,
				   struct flowi6 *fl6,
				   struct neighbour **out_n,
				   u8 *out_ttl)
{
	struct dst_entry *dst;
	struct neighbour *n;

	int ret;

	ret = ipv6_stub->ipv6_dst_lookup(dev_net(mirred_dev), NULL, &dst,
					 fl6);
	if (ret < 0)
		return ret;

	if (!(*out_ttl))
		*out_ttl = ip6_dst_hoplimit(dst);

	ret = get_route_and_out_devs(priv, dst->dev, route_dev, out_dev);
	if (ret < 0) {
		dst_release(dst);
		return ret;
	}

	n = dst_neigh_lookup(dst, &fl6->daddr);
	dst_release(dst);
	if (!n)
		return -ENOMEM;

	*out_n = n;
	return 0;
}