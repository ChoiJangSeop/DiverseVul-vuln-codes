struct xenvif *xenvif_alloc(struct device *parent, domid_t domid,
			    unsigned int handle)
{
	int err;
	struct net_device *dev;
	struct xenvif *vif;
	char name[IFNAMSIZ] = {};
	int i;

	snprintf(name, IFNAMSIZ - 1, "vif%u.%u", domid, handle);
	dev = alloc_netdev(sizeof(struct xenvif), name, ether_setup);
	if (dev == NULL) {
		pr_warn("Could not allocate netdev for %s\n", name);
		return ERR_PTR(-ENOMEM);
	}

	SET_NETDEV_DEV(dev, parent);

	vif = netdev_priv(dev);

	vif->grant_copy_op = vmalloc(sizeof(struct gnttab_copy) *
				     MAX_GRANT_COPY_OPS);
	if (vif->grant_copy_op == NULL) {
		pr_warn("Could not allocate grant copy space for %s\n", name);
		free_netdev(dev);
		return ERR_PTR(-ENOMEM);
	}

	vif->domid  = domid;
	vif->handle = handle;
	vif->can_sg = 1;
	vif->ip_csum = 1;
	vif->dev = dev;

	vif->credit_bytes = vif->remaining_credit = ~0UL;
	vif->credit_usec  = 0UL;
	init_timer(&vif->credit_timeout);
	vif->credit_window_start = get_jiffies_64();

	init_timer(&vif->wake_queue);

	dev->netdev_ops	= &xenvif_netdev_ops;
	dev->hw_features = NETIF_F_SG |
		NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
		NETIF_F_TSO | NETIF_F_TSO6;
	dev->features = dev->hw_features | NETIF_F_RXCSUM;
	SET_ETHTOOL_OPS(dev, &xenvif_ethtool_ops);

	dev->tx_queue_len = XENVIF_QUEUE_LENGTH;

	skb_queue_head_init(&vif->rx_queue);
	skb_queue_head_init(&vif->tx_queue);

	vif->pending_cons = 0;
	vif->pending_prod = MAX_PENDING_REQS;
	for (i = 0; i < MAX_PENDING_REQS; i++)
		vif->pending_ring[i] = i;
	spin_lock_init(&vif->callback_lock);
	spin_lock_init(&vif->response_lock);
	/* If ballooning is disabled, this will consume real memory, so you
	 * better enable it. The long term solution would be to use just a
	 * bunch of valid page descriptors, without dependency on ballooning
	 */
	err = alloc_xenballooned_pages(MAX_PENDING_REQS,
				       vif->mmap_pages,
				       false);
	if (err) {
		netdev_err(dev, "Could not reserve mmap_pages\n");
		return ERR_PTR(-ENOMEM);
	}
	for (i = 0; i < MAX_PENDING_REQS; i++) {
		vif->pending_tx_info[i].callback_struct = (struct ubuf_info)
			{ .callback = xenvif_zerocopy_callback,
			  .ctx = NULL,
			  .desc = i };
		vif->grant_tx_handle[i] = NETBACK_INVALID_HANDLE;
	}

	/*
	 * Initialise a dummy MAC address. We choose the numerically
	 * largest non-broadcast address to prevent the address getting
	 * stolen by an Ethernet bridge for STP purposes.
	 * (FE:FF:FF:FF:FF:FF)
	 */
	memset(dev->dev_addr, 0xFF, ETH_ALEN);
	dev->dev_addr[0] &= ~0x01;

	netif_napi_add(dev, &vif->napi, xenvif_poll, XENVIF_NAPI_WEIGHT);

	netif_carrier_off(dev);

	err = register_netdev(dev);
	if (err) {
		netdev_warn(dev, "Could not register device: err=%d\n", err);
		free_netdev(dev);
		return ERR_PTR(err);
	}

	netdev_dbg(dev, "Successfully created xenvif\n");

	__module_get(THIS_MODULE);

	return vif;
}