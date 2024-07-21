static int bcm_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct net *net;
	struct bcm_sock *bo;
	struct bcm_op *op, *next;

	if (!sk)
		return 0;

	net = sock_net(sk);
	bo = bcm_sk(sk);

	/* remove bcm_ops, timer, rx_unregister(), etc. */

	spin_lock(&bcm_notifier_lock);
	while (bcm_busy_notifier == bo) {
		spin_unlock(&bcm_notifier_lock);
		schedule_timeout_uninterruptible(1);
		spin_lock(&bcm_notifier_lock);
	}
	list_del(&bo->notifier);
	spin_unlock(&bcm_notifier_lock);

	lock_sock(sk);

	list_for_each_entry_safe(op, next, &bo->tx_ops, list)
		bcm_remove_op(op);

	list_for_each_entry_safe(op, next, &bo->rx_ops, list) {
		/*
		 * Don't care if we're bound or not (due to netdev problems)
		 * can_rx_unregister() is always a save thing to do here.
		 */
		if (op->ifindex) {
			/*
			 * Only remove subscriptions that had not
			 * been removed due to NETDEV_UNREGISTER
			 * in bcm_notifier()
			 */
			if (op->rx_reg_dev) {
				struct net_device *dev;

				dev = dev_get_by_index(net, op->ifindex);
				if (dev) {
					bcm_rx_unreg(dev, op);
					dev_put(dev);
				}
			}
		} else
			can_rx_unregister(net, NULL, op->can_id,
					  REGMASK(op->can_id),
					  bcm_rx_handler, op);

		bcm_remove_op(op);
	}

#if IS_ENABLED(CONFIG_PROC_FS)
	/* remove procfs entry */
	if (net->can.bcmproc_dir && bo->bcm_proc_read)
		remove_proc_entry(bo->procname, net->can.bcmproc_dir);
#endif /* CONFIG_PROC_FS */

	/* remove device reference */
	if (bo->bound) {
		bo->bound   = 0;
		bo->ifindex = 0;
	}

	sock_orphan(sk);
	sock->sk = NULL;

	release_sock(sk);
	sock_put(sk);

	return 0;
}