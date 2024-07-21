static int l2tp_eth_create(struct net *net, u32 tunnel_id, u32 session_id, u32 peer_session_id, struct l2tp_session_cfg *cfg)
{
	unsigned char name_assign_type;
	struct net_device *dev;
	char name[IFNAMSIZ];
	struct l2tp_tunnel *tunnel;
	struct l2tp_session *session;
	struct l2tp_eth *priv;
	struct l2tp_eth_sess *spriv;
	int rc;
	struct l2tp_eth_net *pn;

	tunnel = l2tp_tunnel_find(net, tunnel_id);
	if (!tunnel) {
		rc = -ENODEV;
		goto out;
	}

	if (cfg->ifname) {
		strlcpy(name, cfg->ifname, IFNAMSIZ);
		name_assign_type = NET_NAME_USER;
	} else {
		strcpy(name, L2TP_ETH_DEV_NAME);
		name_assign_type = NET_NAME_ENUM;
	}

	session = l2tp_session_create(sizeof(*spriv), tunnel, session_id,
				      peer_session_id, cfg);
	if (IS_ERR(session)) {
		rc = PTR_ERR(session);
		goto out;
	}

	dev = alloc_netdev(sizeof(*priv), name, name_assign_type,
			   l2tp_eth_dev_setup);
	if (!dev) {
		rc = -ENOMEM;
		goto out_del_session;
	}

	dev_net_set(dev, net);
	dev->min_mtu = 0;
	dev->max_mtu = ETH_MAX_MTU;
	l2tp_eth_adjust_mtu(tunnel, session, dev);

	priv = netdev_priv(dev);
	priv->dev = dev;
	priv->session = session;
	INIT_LIST_HEAD(&priv->list);

	priv->tunnel_sock = tunnel->sock;
	session->recv_skb = l2tp_eth_dev_recv;
	session->session_close = l2tp_eth_delete;
#if IS_ENABLED(CONFIG_L2TP_DEBUGFS)
	session->show = l2tp_eth_show;
#endif

	spriv = l2tp_session_priv(session);
	spriv->dev = dev;

	rc = register_netdev(dev);
	if (rc < 0)
		goto out_del_dev;

	__module_get(THIS_MODULE);
	/* Must be done after register_netdev() */
	strlcpy(session->ifname, dev->name, IFNAMSIZ);

	dev_hold(dev);
	pn = l2tp_eth_pernet(dev_net(dev));
	spin_lock(&pn->l2tp_eth_lock);
	list_add(&priv->list, &pn->l2tp_eth_dev_list);
	spin_unlock(&pn->l2tp_eth_lock);

	return 0;

out_del_dev:
	free_netdev(dev);
	spriv->dev = NULL;
out_del_session:
	l2tp_session_delete(session);
out:
	return rc;
}