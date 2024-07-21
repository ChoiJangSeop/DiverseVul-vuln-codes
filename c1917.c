static int tc_ctl_tclass(struct sk_buff *skb, struct nlmsghdr *n)
{
	struct net *net = sock_net(skb->sk);
	struct tcmsg *tcm = nlmsg_data(n);
	struct nlattr *tca[TCA_MAX + 1];
	struct net_device *dev;
	struct Qdisc *q = NULL;
	const struct Qdisc_class_ops *cops;
	unsigned long cl = 0;
	unsigned long new_cl;
	u32 portid;
	u32 clid;
	u32 qid;
	int err;

	if ((n->nlmsg_type != RTM_GETTCLASS) && !capable(CAP_NET_ADMIN))
		return -EPERM;

	err = nlmsg_parse(n, sizeof(*tcm), tca, TCA_MAX, NULL);
	if (err < 0)
		return err;

	dev = __dev_get_by_index(net, tcm->tcm_ifindex);
	if (!dev)
		return -ENODEV;

	/*
	   parent == TC_H_UNSPEC - unspecified parent.
	   parent == TC_H_ROOT   - class is root, which has no parent.
	   parent == X:0	 - parent is root class.
	   parent == X:Y	 - parent is a node in hierarchy.
	   parent == 0:Y	 - parent is X:Y, where X:0 is qdisc.

	   handle == 0:0	 - generate handle from kernel pool.
	   handle == 0:Y	 - class is X:Y, where X:0 is qdisc.
	   handle == X:Y	 - clear.
	   handle == X:0	 - root class.
	 */

	/* Step 1. Determine qdisc handle X:0 */

	portid = tcm->tcm_parent;
	clid = tcm->tcm_handle;
	qid = TC_H_MAJ(clid);

	if (portid != TC_H_ROOT) {
		u32 qid1 = TC_H_MAJ(portid);

		if (qid && qid1) {
			/* If both majors are known, they must be identical. */
			if (qid != qid1)
				return -EINVAL;
		} else if (qid1) {
			qid = qid1;
		} else if (qid == 0)
			qid = dev->qdisc->handle;

		/* Now qid is genuine qdisc handle consistent
		 * both with parent and child.
		 *
		 * TC_H_MAJ(portid) still may be unspecified, complete it now.
		 */
		if (portid)
			portid = TC_H_MAKE(qid, portid);
	} else {
		if (qid == 0)
			qid = dev->qdisc->handle;
	}

	/* OK. Locate qdisc */
	q = qdisc_lookup(dev, qid);
	if (!q)
		return -ENOENT;

	/* An check that it supports classes */
	cops = q->ops->cl_ops;
	if (cops == NULL)
		return -EINVAL;

	/* Now try to get class */
	if (clid == 0) {
		if (portid == TC_H_ROOT)
			clid = qid;
	} else
		clid = TC_H_MAKE(qid, clid);

	if (clid)
		cl = cops->get(q, clid);

	if (cl == 0) {
		err = -ENOENT;
		if (n->nlmsg_type != RTM_NEWTCLASS ||
		    !(n->nlmsg_flags & NLM_F_CREATE))
			goto out;
	} else {
		switch (n->nlmsg_type) {
		case RTM_NEWTCLASS:
			err = -EEXIST;
			if (n->nlmsg_flags & NLM_F_EXCL)
				goto out;
			break;
		case RTM_DELTCLASS:
			err = -EOPNOTSUPP;
			if (cops->delete)
				err = cops->delete(q, cl);
			if (err == 0)
				tclass_notify(net, skb, n, q, cl, RTM_DELTCLASS);
			goto out;
		case RTM_GETTCLASS:
			err = tclass_notify(net, skb, n, q, cl, RTM_NEWTCLASS);
			goto out;
		default:
			err = -EINVAL;
			goto out;
		}
	}

	new_cl = cl;
	err = -EOPNOTSUPP;
	if (cops->change)
		err = cops->change(q, clid, portid, tca, &new_cl);
	if (err == 0)
		tclass_notify(net, skb, n, q, new_cl, RTM_NEWTCLASS);

out:
	if (cl)
		cops->put(q, cl);

	return err;
}