fill_info_if_stats(void *data, interface_info_t *interface_info)
{
	struct info_if_stats **ifsp = (struct info_if_stats **)data;
	struct info_if_stats *ifs = *ifsp;
	endpt *ep = interface_info->ep;
	
	ZERO(*ifs);
	
	if (IS_IPV6(&ep->sin)) {
		if (!client_v6_capable) {
			return;
		}
		ifs->v6_flag = 1;
		ifs->unaddr.addr6 = SOCK_ADDR6(&ep->sin);
		ifs->unbcast.addr6 = SOCK_ADDR6(&ep->bcast);
		ifs->unmask.addr6 = SOCK_ADDR6(&ep->mask);
	} else {
		ifs->v6_flag = 0;
		ifs->unaddr.addr = SOCK_ADDR4(&ep->sin);
		ifs->unbcast.addr = SOCK_ADDR4(&ep->bcast);
		ifs->unmask.addr = SOCK_ADDR4(&ep->mask);
	}
	ifs->v6_flag = htonl(ifs->v6_flag);
	strlcpy(ifs->name, ep->name, sizeof(ifs->name));
	ifs->family = htons(ep->family);
	ifs->flags = htonl(ep->flags);
	ifs->last_ttl = htonl(ep->last_ttl);
	ifs->num_mcast = htonl(ep->num_mcast);
	ifs->received = htonl(ep->received);
	ifs->sent = htonl(ep->sent);
	ifs->notsent = htonl(ep->notsent);
	ifs->ifindex = htonl(ep->ifindex);
	/* scope no longer in endpt, in in6_addr typically */
	ifs->scopeid = ifs->ifindex;
	ifs->ifnum = htonl(ep->ifnum);
	ifs->uptime = htonl(current_time - ep->starttime);
	ifs->ignore_packets = ep->ignore_packets;
	ifs->peercnt = htonl(ep->peercnt);
	ifs->action = interface_info->action;
	
	*ifsp = (struct info_if_stats *)more_pkt();
}