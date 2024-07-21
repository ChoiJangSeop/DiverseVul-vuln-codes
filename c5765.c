static int xt_osf_add_callback(struct net *net, struct sock *ctnl,
			       struct sk_buff *skb, const struct nlmsghdr *nlh,
			       const struct nlattr * const osf_attrs[],
			       struct netlink_ext_ack *extack)
{
	struct xt_osf_user_finger *f;
	struct xt_osf_finger *kf = NULL, *sf;
	int err = 0;

	if (!osf_attrs[OSF_ATTR_FINGER])
		return -EINVAL;

	if (!(nlh->nlmsg_flags & NLM_F_CREATE))
		return -EINVAL;

	f = nla_data(osf_attrs[OSF_ATTR_FINGER]);

	kf = kmalloc(sizeof(struct xt_osf_finger), GFP_KERNEL);
	if (!kf)
		return -ENOMEM;

	memcpy(&kf->finger, f, sizeof(struct xt_osf_user_finger));

	list_for_each_entry(sf, &xt_osf_fingers[!!f->df], finger_entry) {
		if (memcmp(&sf->finger, f, sizeof(struct xt_osf_user_finger)))
			continue;

		kfree(kf);
		kf = NULL;

		if (nlh->nlmsg_flags & NLM_F_EXCL)
			err = -EEXIST;
		break;
	}

	/*
	 * We are protected by nfnl mutex.
	 */
	if (kf)
		list_add_tail_rcu(&kf->finger_entry, &xt_osf_fingers[!!f->df]);

	return err;
}