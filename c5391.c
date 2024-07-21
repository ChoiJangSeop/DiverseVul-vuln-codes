int register_netdevice(struct net_device *dev)
{
	struct hlist_head *head;
	struct hlist_node *p;
	int ret;

	BUG_ON(dev_boot_phase);
	ASSERT_RTNL();

	/* When net_device's are persistent, this will be fatal. */
	BUG_ON(dev->reg_state != NETREG_UNINITIALIZED);

	spin_lock_init(&dev->queue_lock);
	spin_lock_init(&dev->xmit_lock);
	dev->xmit_lock_owner = -1;
#ifdef CONFIG_NET_CLS_ACT
	spin_lock_init(&dev->ingress_lock);
#endif

	ret = alloc_divert_blk(dev);
	if (ret)
		goto out;

	dev->iflink = -1;

	/* Init, if this function is available */
	if (dev->init) {
		ret = dev->init(dev);
		if (ret) {
			if (ret > 0)
				ret = -EIO;
			goto out_err;
		}
	}
 
	if (!dev_valid_name(dev->name)) {
		ret = -EINVAL;
		goto out_err;
	}

	dev->ifindex = dev_new_index();
	if (dev->iflink == -1)
		dev->iflink = dev->ifindex;

	/* Check for existence of name */
	head = dev_name_hash(dev->name);
	hlist_for_each(p, head) {
		struct net_device *d
			= hlist_entry(p, struct net_device, name_hlist);
		if (!strncmp(d->name, dev->name, IFNAMSIZ)) {
			ret = -EEXIST;
 			goto out_err;
		}
 	}

	/* Fix illegal SG+CSUM combinations. */
	if ((dev->features & NETIF_F_SG) &&
	    !(dev->features & (NETIF_F_IP_CSUM |
			       NETIF_F_NO_CSUM |
			       NETIF_F_HW_CSUM))) {
		printk("%s: Dropping NETIF_F_SG since no checksum feature.\n",
		       dev->name);
		dev->features &= ~NETIF_F_SG;
	}

	/* TSO requires that SG is present as well. */
	if ((dev->features & NETIF_F_TSO) &&
	    !(dev->features & NETIF_F_SG)) {
		printk("%s: Dropping NETIF_F_TSO since no SG feature.\n",
		       dev->name);
		dev->features &= ~NETIF_F_TSO;
	}

	/*
	 *	nil rebuild_header routine,
	 *	that should be never called and used as just bug trap.
	 */

	if (!dev->rebuild_header)
		dev->rebuild_header = default_rebuild_header;

	/*
	 *	Default initial state at registry is that the
	 *	device is present.
	 */

	set_bit(__LINK_STATE_PRESENT, &dev->state);

	dev->next = NULL;
	dev_init_scheduler(dev);
	write_lock_bh(&dev_base_lock);
	*dev_tail = dev;
	dev_tail = &dev->next;
	hlist_add_head(&dev->name_hlist, head);
	hlist_add_head(&dev->index_hlist, dev_index_hash(dev->ifindex));
	dev_hold(dev);
	dev->reg_state = NETREG_REGISTERING;
	write_unlock_bh(&dev_base_lock);

	/* Notify protocols, that a new device appeared. */
	notifier_call_chain(&netdev_chain, NETDEV_REGISTER, dev);

	/* Finish registration after unlock */
	net_set_todo(dev);
	ret = 0;

out:
	return ret;
out_err:
	free_divert_blk(dev);
	goto out;
}