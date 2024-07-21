static int ax25_rt_opt(struct ax25_route_opt_struct *rt_option)
{
	ax25_route *ax25_rt;
	ax25_dev *ax25_dev;
	int err = 0;

	if ((ax25_dev = ax25_addr_ax25dev(&rt_option->port_addr)) == NULL)
		return -EINVAL;

	write_lock_bh(&ax25_route_lock);

	ax25_rt = ax25_route_list;
	while (ax25_rt != NULL) {
		if (ax25_rt->dev == ax25_dev->dev &&
		    ax25cmp(&rt_option->dest_addr, &ax25_rt->callsign) == 0) {
			switch (rt_option->cmd) {
			case AX25_SET_RT_IPMODE:
				switch (rt_option->arg) {
				case ' ':
				case 'D':
				case 'V':
					ax25_rt->ip_mode = rt_option->arg;
					break;
				default:
					err = -EINVAL;
					goto out;
				}
				break;
			default:
				err = -EINVAL;
				goto out;
			}
		}
		ax25_rt = ax25_rt->next;
	}

out:
	ax25_dev_put(ax25_dev);
	write_unlock_bh(&ax25_route_lock);
	return err;
}