static int inet6_fill_prefix(struct sk_buff *skb, struct inet6_dev *idev,
			struct prefix_info *pinfo, u32 pid, u32 seq, 
			int event, unsigned int flags)
{
	struct prefixmsg	*pmsg;
	struct nlmsghdr 	*nlh;
	unsigned char		*b = skb->tail;
	struct prefix_cacheinfo	ci;

	nlh = NLMSG_NEW(skb, pid, seq, event, sizeof(*pmsg), flags);
	pmsg = NLMSG_DATA(nlh);
	pmsg->prefix_family = AF_INET6;
	pmsg->prefix_ifindex = idev->dev->ifindex;
	pmsg->prefix_len = pinfo->prefix_len;
	pmsg->prefix_type = pinfo->type;
	
	pmsg->prefix_flags = 0;
	if (pinfo->onlink)
		pmsg->prefix_flags |= IF_PREFIX_ONLINK;
	if (pinfo->autoconf)
		pmsg->prefix_flags |= IF_PREFIX_AUTOCONF;

	RTA_PUT(skb, PREFIX_ADDRESS, sizeof(pinfo->prefix), &pinfo->prefix);

	ci.preferred_time = ntohl(pinfo->prefered);
	ci.valid_time = ntohl(pinfo->valid);
	RTA_PUT(skb, PREFIX_CACHEINFO, sizeof(ci), &ci);

	nlh->nlmsg_len = skb->tail - b;
	return skb->len;

nlmsg_failure:
rtattr_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}