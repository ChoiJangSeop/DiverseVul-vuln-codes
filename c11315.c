void ax25_dev_device_up(struct net_device *dev)
{
	ax25_dev *ax25_dev;

	if ((ax25_dev = kzalloc(sizeof(*ax25_dev), GFP_ATOMIC)) == NULL) {
		printk(KERN_ERR "AX.25: ax25_dev_device_up - out of memory\n");
		return;
	}

	dev->ax25_ptr     = ax25_dev;
	ax25_dev->dev     = dev;
	dev_hold_track(dev, &ax25_dev->dev_tracker, GFP_ATOMIC);
	ax25_dev->forward = NULL;

	ax25_dev->values[AX25_VALUES_IPDEFMODE] = AX25_DEF_IPDEFMODE;
	ax25_dev->values[AX25_VALUES_AXDEFMODE] = AX25_DEF_AXDEFMODE;
	ax25_dev->values[AX25_VALUES_BACKOFF]   = AX25_DEF_BACKOFF;
	ax25_dev->values[AX25_VALUES_CONMODE]   = AX25_DEF_CONMODE;
	ax25_dev->values[AX25_VALUES_WINDOW]    = AX25_DEF_WINDOW;
	ax25_dev->values[AX25_VALUES_EWINDOW]   = AX25_DEF_EWINDOW;
	ax25_dev->values[AX25_VALUES_T1]        = AX25_DEF_T1;
	ax25_dev->values[AX25_VALUES_T2]        = AX25_DEF_T2;
	ax25_dev->values[AX25_VALUES_T3]        = AX25_DEF_T3;
	ax25_dev->values[AX25_VALUES_IDLE]	= AX25_DEF_IDLE;
	ax25_dev->values[AX25_VALUES_N2]        = AX25_DEF_N2;
	ax25_dev->values[AX25_VALUES_PACLEN]	= AX25_DEF_PACLEN;
	ax25_dev->values[AX25_VALUES_PROTOCOL]  = AX25_DEF_PROTOCOL;
	ax25_dev->values[AX25_VALUES_DS_TIMEOUT]= AX25_DEF_DS_TIMEOUT;

#if defined(CONFIG_AX25_DAMA_SLAVE) || defined(CONFIG_AX25_DAMA_MASTER)
	ax25_ds_setup_timer(ax25_dev);
#endif

	spin_lock_bh(&ax25_dev_lock);
	ax25_dev->next = ax25_dev_list;
	ax25_dev_list  = ax25_dev;
	spin_unlock_bh(&ax25_dev_lock);

	ax25_register_dev_sysctl(ax25_dev);
}