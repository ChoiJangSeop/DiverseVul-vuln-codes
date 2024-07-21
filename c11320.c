static int ax25_rt_del(struct ax25_routes_struct *route)
{
	ax25_route *s, *t, *ax25_rt;
	ax25_dev *ax25_dev;

	if ((ax25_dev = ax25_addr_ax25dev(&route->port_addr)) == NULL)
		return -EINVAL;

	write_lock_bh(&ax25_route_lock);

	ax25_rt = ax25_route_list;
	while (ax25_rt != NULL) {
		s       = ax25_rt;
		ax25_rt = ax25_rt->next;
		if (s->dev == ax25_dev->dev &&
		    ax25cmp(&route->dest_addr, &s->callsign) == 0) {
			if (ax25_route_list == s) {
				ax25_route_list = s->next;
				ax25_put_route(s);
			} else {
				for (t = ax25_route_list; t != NULL; t = t->next) {
					if (t->next == s) {
						t->next = s->next;
						ax25_put_route(s);
						break;
					}
				}
			}
		}
	}
	ax25_dev_put(ax25_dev);
	write_unlock_bh(&ax25_route_lock);

	return 0;
}