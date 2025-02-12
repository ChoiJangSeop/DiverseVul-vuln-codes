tc_dump_action(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct nlmsghdr *nlh;
	unsigned char *b = skb->tail;
	struct rtattr *x;
	struct tc_action_ops *a_o;
	struct tc_action a;
	int ret = 0;
	struct tcamsg *t = (struct tcamsg *) NLMSG_DATA(cb->nlh);
	char *kind = find_dump_kind(cb->nlh);

	if (kind == NULL) {
		printk("tc_dump_action: action bad kind\n");
		return 0;
	}

	a_o = tc_lookup_action_n(kind);
	if (a_o == NULL) {
		printk("failed to find %s\n", kind);
		return 0;
	}

	memset(&a, 0, sizeof(struct tc_action));
	a.ops = a_o;

	if (a_o->walk == NULL) {
		printk("tc_dump_action: %s !capable of dumping table\n", kind);
		goto rtattr_failure;
	}

	nlh = NLMSG_PUT(skb, NETLINK_CB(cb->skb).pid, cb->nlh->nlmsg_seq,
	                cb->nlh->nlmsg_type, sizeof(*t));
	t = NLMSG_DATA(nlh);
	t->tca_family = AF_UNSPEC;

	x = (struct rtattr *) skb->tail;
	RTA_PUT(skb, TCA_ACT_TAB, 0, NULL);

	ret = a_o->walk(skb, cb, RTM_GETACTION, &a);
	if (ret < 0)
		goto rtattr_failure;

	if (ret > 0) {
		x->rta_len = skb->tail - (u8 *) x;
		ret = skb->len;
	} else
		skb_trim(skb, (u8*)x - skb->data);

	nlh->nlmsg_len = skb->tail - b;
	if (NETLINK_CB(cb->skb).pid && ret)
		nlh->nlmsg_flags |= NLM_F_MULTI;
	module_put(a_o->owner);
	return skb->len;

rtattr_failure:
nlmsg_failure:
	module_put(a_o->owner);
	skb_trim(skb, b - skb->data);
	return skb->len;
}