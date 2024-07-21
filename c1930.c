static int dn_fib_rtm_newroute(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	struct net *net = sock_net(skb->sk);
	struct dn_fib_table *tb;
	struct rtmsg *r = nlmsg_data(nlh);
	struct nlattr *attrs[RTA_MAX+1];
	int err;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!net_eq(net, &init_net))
		return -EINVAL;

	err = nlmsg_parse(nlh, sizeof(*r), attrs, RTA_MAX, rtm_dn_policy);
	if (err < 0)
		return err;

	tb = dn_fib_get_table(rtm_get_table(attrs, r->rtm_table), 1);
	if (!tb)
		return -ENOBUFS;

	return tb->insert(tb, r, attrs, nlh, &NETLINK_CB(skb));
}