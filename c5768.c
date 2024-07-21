static int nfnl_cthelper_del(struct net *net, struct sock *nfnl,
			     struct sk_buff *skb, const struct nlmsghdr *nlh,
			     const struct nlattr * const tb[],
			     struct netlink_ext_ack *extack)
{
	char *helper_name = NULL;
	struct nf_conntrack_helper *cur;
	struct nf_conntrack_tuple tuple;
	bool tuple_set = false, found = false;
	struct nfnl_cthelper *nlcth, *n;
	int j = 0, ret;

	if (tb[NFCTH_NAME])
		helper_name = nla_data(tb[NFCTH_NAME]);

	if (tb[NFCTH_TUPLE]) {
		ret = nfnl_cthelper_parse_tuple(&tuple, tb[NFCTH_TUPLE]);
		if (ret < 0)
			return ret;

		tuple_set = true;
	}

	ret = -ENOENT;
	list_for_each_entry_safe(nlcth, n, &nfnl_cthelper_list, list) {
		cur = &nlcth->helper;
		j++;

		if (helper_name &&
		    strncmp(cur->name, helper_name, NF_CT_HELPER_NAME_LEN))
			continue;

		if (tuple_set &&
		    (tuple.src.l3num != cur->tuple.src.l3num ||
		     tuple.dst.protonum != cur->tuple.dst.protonum))
			continue;

		if (refcount_dec_if_one(&cur->refcnt)) {
			found = true;
			nf_conntrack_helper_unregister(cur);
			kfree(cur->expect_policy);

			list_del(&nlcth->list);
			kfree(nlcth);
		} else {
			ret = -EBUSY;
		}
	}

	/* Make sure we return success if we flush and there is no helpers */
	return (found || j == 0) ? 0 : ret;
}