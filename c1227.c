static int dcbnl_cee_fill(struct sk_buff *skb, struct net_device *netdev)
{
	struct nlattr *cee, *app;
	struct dcb_app_type *itr;
	const struct dcbnl_rtnl_ops *ops = netdev->dcbnl_ops;
	int dcbx, i, err = -EMSGSIZE;
	u8 value;

	if (nla_put_string(skb, DCB_ATTR_IFNAME, netdev->name))
		goto nla_put_failure;
	cee = nla_nest_start(skb, DCB_ATTR_CEE);
	if (!cee)
		goto nla_put_failure;

	/* local pg */
	if (ops->getpgtccfgtx && ops->getpgbwgcfgtx) {
		err = dcbnl_cee_pg_fill(skb, netdev, 1);
		if (err)
			goto nla_put_failure;
	}

	if (ops->getpgtccfgrx && ops->getpgbwgcfgrx) {
		err = dcbnl_cee_pg_fill(skb, netdev, 0);
		if (err)
			goto nla_put_failure;
	}

	/* local pfc */
	if (ops->getpfccfg) {
		struct nlattr *pfc_nest = nla_nest_start(skb, DCB_ATTR_CEE_PFC);

		if (!pfc_nest)
			goto nla_put_failure;

		for (i = DCB_PFC_UP_ATTR_0; i <= DCB_PFC_UP_ATTR_7; i++) {
			ops->getpfccfg(netdev, i - DCB_PFC_UP_ATTR_0, &value);
			if (nla_put_u8(skb, i, value))
				goto nla_put_failure;
		}
		nla_nest_end(skb, pfc_nest);
	}

	/* local app */
	spin_lock(&dcb_lock);
	app = nla_nest_start(skb, DCB_ATTR_CEE_APP_TABLE);
	if (!app)
		goto dcb_unlock;

	list_for_each_entry(itr, &dcb_app_list, list) {
		if (itr->ifindex == netdev->ifindex) {
			struct nlattr *app_nest = nla_nest_start(skb,
								 DCB_ATTR_APP);
			if (!app_nest)
				goto dcb_unlock;

			err = nla_put_u8(skb, DCB_APP_ATTR_IDTYPE,
					 itr->app.selector);
			if (err)
				goto dcb_unlock;

			err = nla_put_u16(skb, DCB_APP_ATTR_ID,
					  itr->app.protocol);
			if (err)
				goto dcb_unlock;

			err = nla_put_u8(skb, DCB_APP_ATTR_PRIORITY,
					 itr->app.priority);
			if (err)
				goto dcb_unlock;

			nla_nest_end(skb, app_nest);
		}
	}
	nla_nest_end(skb, app);

	if (netdev->dcbnl_ops->getdcbx)
		dcbx = netdev->dcbnl_ops->getdcbx(netdev);
	else
		dcbx = -EOPNOTSUPP;

	spin_unlock(&dcb_lock);

	/* features flags */
	if (ops->getfeatcfg) {
		struct nlattr *feat = nla_nest_start(skb, DCB_ATTR_CEE_FEAT);
		if (!feat)
			goto nla_put_failure;

		for (i = DCB_FEATCFG_ATTR_ALL + 1; i <= DCB_FEATCFG_ATTR_MAX;
		     i++)
			if (!ops->getfeatcfg(netdev, i, &value) &&
			    nla_put_u8(skb, i, value))
				goto nla_put_failure;

		nla_nest_end(skb, feat);
	}

	/* peer info if available */
	if (ops->cee_peer_getpg) {
		struct cee_pg pg;
		err = ops->cee_peer_getpg(netdev, &pg);
		if (!err &&
		    nla_put(skb, DCB_ATTR_CEE_PEER_PG, sizeof(pg), &pg))
			goto nla_put_failure;
	}

	if (ops->cee_peer_getpfc) {
		struct cee_pfc pfc;
		err = ops->cee_peer_getpfc(netdev, &pfc);
		if (!err &&
		    nla_put(skb, DCB_ATTR_CEE_PEER_PFC, sizeof(pfc), &pfc))
			goto nla_put_failure;
	}

	if (ops->peer_getappinfo && ops->peer_getapptable) {
		err = dcbnl_build_peer_app(netdev, skb,
					   DCB_ATTR_CEE_PEER_APP_TABLE,
					   DCB_ATTR_CEE_PEER_APP_INFO,
					   DCB_ATTR_CEE_PEER_APP);
		if (err)
			goto nla_put_failure;
	}
	nla_nest_end(skb, cee);

	/* DCBX state */
	if (dcbx >= 0) {
		err = nla_put_u8(skb, DCB_ATTR_DCBX, dcbx);
		if (err)
			goto nla_put_failure;
	}
	return 0;

dcb_unlock:
	spin_unlock(&dcb_lock);
nla_put_failure:
	return err;
}