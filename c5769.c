static int nfnl_cthelper_get(struct net *net, struct sock *nfnl,
			     struct sk_buff *skb, const struct nlmsghdr *nlh,
			     const struct nlattr * const tb[],
			     struct netlink_ext_ack *extack)
{
	int ret = -ENOENT;
	struct nf_conntrack_helper *cur;
	struct sk_buff *skb2;
	char *helper_name = NULL;
	struct nf_conntrack_tuple tuple;
	struct nfnl_cthelper *nlcth;
	bool tuple_set = false;

	if (nlh->nlmsg_flags & NLM_F_DUMP) {
		struct netlink_dump_control c = {
			.dump = nfnl_cthelper_dump_table,
		};
		return netlink_dump_start(nfnl, skb, nlh, &c);
	}

	if (tb[NFCTH_NAME])
		helper_name = nla_data(tb[NFCTH_NAME]);

	if (tb[NFCTH_TUPLE]) {
		ret = nfnl_cthelper_parse_tuple(&tuple, tb[NFCTH_TUPLE]);
		if (ret < 0)
			return ret;

		tuple_set = true;
	}

	list_for_each_entry(nlcth, &nfnl_cthelper_list, list) {
		cur = &nlcth->helper;
		if (helper_name &&
		    strncmp(cur->name, helper_name, NF_CT_HELPER_NAME_LEN))
			continue;

		if (tuple_set &&
		    (tuple.src.l3num != cur->tuple.src.l3num ||
		     tuple.dst.protonum != cur->tuple.dst.protonum))
			continue;

		skb2 = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
		if (skb2 == NULL) {
			ret = -ENOMEM;
			break;
		}

		ret = nfnl_cthelper_fill_info(skb2, NETLINK_CB(skb).portid,
					      nlh->nlmsg_seq,
					      NFNL_MSG_TYPE(nlh->nlmsg_type),
					      NFNL_MSG_CTHELPER_NEW, cur);
		if (ret <= 0) {
			kfree_skb(skb2);
			break;
		}

		ret = netlink_unicast(nfnl, skb2, NETLINK_CB(skb).portid,
				      MSG_DONTWAIT);
		if (ret > 0)
			ret = 0;

		/* this avoids a loop in nfnetlink. */
		return ret == -EAGAIN ? -ENOBUFS : ret;
	}
	return ret;
}