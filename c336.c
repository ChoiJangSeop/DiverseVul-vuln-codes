static int rtnetlink_fill_ifinfo(struct sk_buff *skb, struct net_device *dev,
				 int type, u32 pid, u32 seq, u32 change, 
				 unsigned int flags)
{
	struct ifinfomsg *r;
	struct nlmsghdr  *nlh;
	unsigned char	 *b = skb->tail;

	nlh = NLMSG_NEW(skb, pid, seq, type, sizeof(*r), flags);
	r = NLMSG_DATA(nlh);
	r->ifi_family = AF_UNSPEC;
	r->ifi_type = dev->type;
	r->ifi_index = dev->ifindex;
	r->ifi_flags = dev_get_flags(dev);
	r->ifi_change = change;

	RTA_PUT(skb, IFLA_IFNAME, strlen(dev->name)+1, dev->name);

	if (1) {
		u32 txqlen = dev->tx_queue_len;
		RTA_PUT(skb, IFLA_TXQLEN, sizeof(txqlen), &txqlen);
	}

	if (1) {
		u32 weight = dev->weight;
		RTA_PUT(skb, IFLA_WEIGHT, sizeof(weight), &weight);
	}

	if (1) {
		struct rtnl_link_ifmap map = {
			.mem_start   = dev->mem_start,
			.mem_end     = dev->mem_end,
			.base_addr   = dev->base_addr,
			.irq         = dev->irq,
			.dma         = dev->dma,
			.port        = dev->if_port,
		};
		RTA_PUT(skb, IFLA_MAP, sizeof(map), &map);
	}

	if (dev->addr_len) {
		RTA_PUT(skb, IFLA_ADDRESS, dev->addr_len, dev->dev_addr);
		RTA_PUT(skb, IFLA_BROADCAST, dev->addr_len, dev->broadcast);
	}

	if (1) {
		u32 mtu = dev->mtu;
		RTA_PUT(skb, IFLA_MTU, sizeof(mtu), &mtu);
	}

	if (dev->ifindex != dev->iflink) {
		u32 iflink = dev->iflink;
		RTA_PUT(skb, IFLA_LINK, sizeof(iflink), &iflink);
	}

	if (dev->qdisc_sleeping)
		RTA_PUT(skb, IFLA_QDISC,
			strlen(dev->qdisc_sleeping->ops->id) + 1,
			dev->qdisc_sleeping->ops->id);
	
	if (dev->master) {
		u32 master = dev->master->ifindex;
		RTA_PUT(skb, IFLA_MASTER, sizeof(master), &master);
	}

	if (dev->get_stats) {
		unsigned long *stats = (unsigned long*)dev->get_stats(dev);
		if (stats) {
			struct rtattr  *a;
			__u32	       *s;
			int		i;
			int		n = sizeof(struct rtnl_link_stats)/4;

			a = __RTA_PUT(skb, IFLA_STATS, n*4);
			s = RTA_DATA(a);
			for (i=0; i<n; i++)
				s[i] = stats[i];
		}
	}
	nlh->nlmsg_len = skb->tail - b;
	return skb->len;

nlmsg_failure:
rtattr_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}