void ax25_dev_device_down(struct net_device *dev)
{
	ax25_dev *s, *ax25_dev;

	if ((ax25_dev = ax25_dev_ax25dev(dev)) == NULL)
		return;

	ax25_unregister_dev_sysctl(ax25_dev);

	spin_lock_bh(&ax25_dev_lock);

#ifdef CONFIG_AX25_DAMA_SLAVE
	ax25_ds_del_timer(ax25_dev);
#endif

	/*
	 *	Remove any packet forwarding that points to this device.
	 */
	for (s = ax25_dev_list; s != NULL; s = s->next)
		if (s->forward == dev)
			s->forward = NULL;

	if ((s = ax25_dev_list) == ax25_dev) {
		ax25_dev_list = s->next;
		ax25_dev_put(ax25_dev);
		spin_unlock_bh(&ax25_dev_lock);
		dev->ax25_ptr = NULL;
		dev_put_track(dev, &ax25_dev->dev_tracker);
		ax25_dev_put(ax25_dev);
		return;
	}

	while (s != NULL && s->next != NULL) {
		if (s->next == ax25_dev) {
			s->next = ax25_dev->next;
			ax25_dev_put(ax25_dev);
			spin_unlock_bh(&ax25_dev_lock);
			dev->ax25_ptr = NULL;
			dev_put_track(dev, &ax25_dev->dev_tracker);
			ax25_dev_put(ax25_dev);
			return;
		}

		s = s->next;
	}
	spin_unlock_bh(&ax25_dev_lock);
	dev->ax25_ptr = NULL;
	ax25_dev_put(ax25_dev);
}