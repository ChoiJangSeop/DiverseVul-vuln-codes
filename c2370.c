static int __mkroute_input(struct sk_buff *skb,
			   const struct fib_result *res,
			   struct in_device *in_dev,
			   __be32 daddr, __be32 saddr, u32 tos)
{
	struct fib_nh_exception *fnhe;
	struct rtable *rth;
	int err;
	struct in_device *out_dev;
	unsigned int flags = 0;
	bool do_cache;
	u32 itag = 0;

	/* get a working reference to the output device */
	out_dev = __in_dev_get_rcu(FIB_RES_DEV(*res));
	if (out_dev == NULL) {
		net_crit_ratelimited("Bug in ip_route_input_slow(). Please report.\n");
		return -EINVAL;
	}

	err = fib_validate_source(skb, saddr, daddr, tos, FIB_RES_OIF(*res),
				  in_dev->dev, in_dev, &itag);
	if (err < 0) {
		ip_handle_martian_source(in_dev->dev, in_dev, skb, daddr,
					 saddr);

		goto cleanup;
	}

	do_cache = res->fi && !itag;
	if (out_dev == in_dev && err && IN_DEV_TX_REDIRECTS(out_dev) &&
	    (IN_DEV_SHARED_MEDIA(out_dev) ||
	     inet_addr_onlink(out_dev, saddr, FIB_RES_GW(*res)))) {
		flags |= RTCF_DOREDIRECT;
		do_cache = false;
	}

	if (skb->protocol != htons(ETH_P_IP)) {
		/* Not IP (i.e. ARP). Do not create route, if it is
		 * invalid for proxy arp. DNAT routes are always valid.
		 *
		 * Proxy arp feature have been extended to allow, ARP
		 * replies back to the same interface, to support
		 * Private VLAN switch technologies. See arp.c.
		 */
		if (out_dev == in_dev &&
		    IN_DEV_PROXY_ARP_PVLAN(in_dev) == 0) {
			err = -EINVAL;
			goto cleanup;
		}
	}

	fnhe = find_exception(&FIB_RES_NH(*res), daddr);
	if (do_cache) {
		if (fnhe != NULL)
			rth = rcu_dereference(fnhe->fnhe_rth_input);
		else
			rth = rcu_dereference(FIB_RES_NH(*res).nh_rth_input);

		if (rt_cache_valid(rth)) {
			skb_dst_set_noref(skb, &rth->dst);
			goto out;
		}
	}

	rth = rt_dst_alloc(out_dev->dev,
			   IN_DEV_CONF_GET(in_dev, NOPOLICY),
			   IN_DEV_CONF_GET(out_dev, NOXFRM), do_cache);
	if (!rth) {
		err = -ENOBUFS;
		goto cleanup;
	}

	rth->rt_genid = rt_genid_ipv4(dev_net(rth->dst.dev));
	rth->rt_flags = flags;
	rth->rt_type = res->type;
	rth->rt_is_input = 1;
	rth->rt_iif 	= 0;
	rth->rt_pmtu	= 0;
	rth->rt_gateway	= 0;
	rth->rt_uses_gateway = 0;
	INIT_LIST_HEAD(&rth->rt_uncached);
	RT_CACHE_STAT_INC(in_slow_tot);

	rth->dst.input = ip_forward;
	rth->dst.output = ip_output;

	rt_set_nexthop(rth, daddr, res, fnhe, res->fi, res->type, itag);
	skb_dst_set(skb, &rth->dst);
out:
	err = 0;
 cleanup:
	return err;
}