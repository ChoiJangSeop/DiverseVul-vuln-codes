static int nfnl_cthelper_new(struct net *net, struct sock *nfnl,
			     struct sk_buff *skb, const struct nlmsghdr *nlh,
			     const struct nlattr * const tb[],
			     struct netlink_ext_ack *extack)
{
	const char *helper_name;
	struct nf_conntrack_helper *cur, *helper = NULL;
	struct nf_conntrack_tuple tuple;
	struct nfnl_cthelper *nlcth;
	int ret = 0;

	if (!tb[NFCTH_NAME] || !tb[NFCTH_TUPLE])
		return -EINVAL;

	helper_name = nla_data(tb[NFCTH_NAME]);

	ret = nfnl_cthelper_parse_tuple(&tuple, tb[NFCTH_TUPLE]);
	if (ret < 0)
		return ret;

	list_for_each_entry(nlcth, &nfnl_cthelper_list, list) {
		cur = &nlcth->helper;

		if (strncmp(cur->name, helper_name, NF_CT_HELPER_NAME_LEN))
			continue;

		if ((tuple.src.l3num != cur->tuple.src.l3num ||
		     tuple.dst.protonum != cur->tuple.dst.protonum))
			continue;

		if (nlh->nlmsg_flags & NLM_F_EXCL)
			return -EEXIST;

		helper = cur;
		break;
	}

	if (helper == NULL)
		ret = nfnl_cthelper_create(tb, &tuple);
	else
		ret = nfnl_cthelper_update(tb, helper);

	return ret;
}