static int dcbnl_ieee_fill(struct sk_buff *skb, struct net_device *netdev)
{
	struct nlattr *ieee, *app;
	struct dcb_app_type *itr;
	const struct dcbnl_rtnl_ops *ops = netdev->dcbnl_ops;
	int dcbx;
	int err;

	if (nla_put_string(skb, DCB_ATTR_IFNAME, netdev->name))
		return -EMSGSIZE;

	ieee = nla_nest_start(skb, DCB_ATTR_IEEE);
	if (!ieee)
		return -EMSGSIZE;

	if (ops->ieee_getets) {
		struct ieee_ets ets;
		err = ops->ieee_getets(netdev, &ets);
		if (!err &&
		    nla_put(skb, DCB_ATTR_IEEE_ETS, sizeof(ets), &ets))
			return -EMSGSIZE;
	}

	if (ops->ieee_getmaxrate) {
		struct ieee_maxrate maxrate;
		err = ops->ieee_getmaxrate(netdev, &maxrate);
		if (!err) {
			err = nla_put(skb, DCB_ATTR_IEEE_MAXRATE,
				      sizeof(maxrate), &maxrate);
			if (err)
				return -EMSGSIZE;
		}
	}

	if (ops->ieee_getpfc) {
		struct ieee_pfc pfc;
		err = ops->ieee_getpfc(netdev, &pfc);
		if (!err &&
		    nla_put(skb, DCB_ATTR_IEEE_PFC, sizeof(pfc), &pfc))
			return -EMSGSIZE;
	}

	app = nla_nest_start(skb, DCB_ATTR_IEEE_APP_TABLE);
	if (!app)
		return -EMSGSIZE;

	spin_lock(&dcb_lock);
	list_for_each_entry(itr, &dcb_app_list, list) {
		if (itr->ifindex == netdev->ifindex) {
			err = nla_put(skb, DCB_ATTR_IEEE_APP, sizeof(itr->app),
					 &itr->app);
			if (err) {
				spin_unlock(&dcb_lock);
				return -EMSGSIZE;
			}
		}
	}

	if (netdev->dcbnl_ops->getdcbx)
		dcbx = netdev->dcbnl_ops->getdcbx(netdev);
	else
		dcbx = -EOPNOTSUPP;

	spin_unlock(&dcb_lock);
	nla_nest_end(skb, app);

	/* get peer info if available */
	if (ops->ieee_peer_getets) {
		struct ieee_ets ets;
		err = ops->ieee_peer_getets(netdev, &ets);
		if (!err &&
		    nla_put(skb, DCB_ATTR_IEEE_PEER_ETS, sizeof(ets), &ets))
			return -EMSGSIZE;
	}

	if (ops->ieee_peer_getpfc) {
		struct ieee_pfc pfc;
		err = ops->ieee_peer_getpfc(netdev, &pfc);
		if (!err &&
		    nla_put(skb, DCB_ATTR_IEEE_PEER_PFC, sizeof(pfc), &pfc))
			return -EMSGSIZE;
	}

	if (ops->peer_getappinfo && ops->peer_getapptable) {
		err = dcbnl_build_peer_app(netdev, skb,
					   DCB_ATTR_IEEE_PEER_APP,
					   DCB_ATTR_IEEE_APP_UNSPEC,
					   DCB_ATTR_IEEE_APP);
		if (err)
			return -EMSGSIZE;
	}

	nla_nest_end(skb, ieee);
	if (dcbx >= 0) {
		err = nla_put_u8(skb, DCB_ATTR_DCBX, dcbx);
		if (err)
			return -EMSGSIZE;
	}

	return 0;
}