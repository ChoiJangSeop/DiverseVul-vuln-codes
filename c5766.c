static int xt_osf_remove_callback(struct net *net, struct sock *ctnl,
				  struct sk_buff *skb,
				  const struct nlmsghdr *nlh,
				  const struct nlattr * const osf_attrs[],
				  struct netlink_ext_ack *extack)
{
	struct xt_osf_user_finger *f;
	struct xt_osf_finger *sf;
	int err = -ENOENT;

	if (!osf_attrs[OSF_ATTR_FINGER])
		return -EINVAL;

	f = nla_data(osf_attrs[OSF_ATTR_FINGER]);

	list_for_each_entry(sf, &xt_osf_fingers[!!f->df], finger_entry) {
		if (memcmp(&sf->finger, f, sizeof(struct xt_osf_user_finger)))
			continue;

		/*
		 * We are protected by nfnl mutex.
		 */
		list_del_rcu(&sf->finger_entry);
		kfree_rcu(sf, rcu_head);

		err = 0;
		break;
	}

	return err;
}