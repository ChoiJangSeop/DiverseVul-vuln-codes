ax25_dev *ax25_addr_ax25dev(ax25_address *addr)
{
	ax25_dev *ax25_dev, *res = NULL;

	spin_lock_bh(&ax25_dev_lock);
	for (ax25_dev = ax25_dev_list; ax25_dev != NULL; ax25_dev = ax25_dev->next)
		if (ax25cmp(addr, (const ax25_address *)ax25_dev->dev->dev_addr) == 0) {
			res = ax25_dev;
		}
	spin_unlock_bh(&ax25_dev_lock);

	return res;
}