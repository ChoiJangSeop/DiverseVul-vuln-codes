parse_ipsecrequest(struct xfrm_policy *xp, struct sadb_x_ipsecrequest *rq)
{
	struct net *net = xp_net(xp);
	struct xfrm_tmpl *t = xp->xfrm_vec + xp->xfrm_nr;
	int mode;

	if (xp->xfrm_nr >= XFRM_MAX_DEPTH)
		return -ELOOP;

	if (rq->sadb_x_ipsecrequest_mode == 0)
		return -EINVAL;

	t->id.proto = rq->sadb_x_ipsecrequest_proto; /* XXX check proto */
	if ((mode = pfkey_mode_to_xfrm(rq->sadb_x_ipsecrequest_mode)) < 0)
		return -EINVAL;
	t->mode = mode;
	if (rq->sadb_x_ipsecrequest_level == IPSEC_LEVEL_USE)
		t->optional = 1;
	else if (rq->sadb_x_ipsecrequest_level == IPSEC_LEVEL_UNIQUE) {
		t->reqid = rq->sadb_x_ipsecrequest_reqid;
		if (t->reqid > IPSEC_MANUAL_REQID_MAX)
			t->reqid = 0;
		if (!t->reqid && !(t->reqid = gen_reqid(net)))
			return -ENOBUFS;
	}

	/* addresses present only in tunnel mode */
	if (t->mode == XFRM_MODE_TUNNEL) {
		int err;

		err = parse_sockaddr_pair(
			(struct sockaddr *)(rq + 1),
			rq->sadb_x_ipsecrequest_len - sizeof(*rq),
			&t->saddr, &t->id.daddr, &t->encap_family);
		if (err)
			return err;
	} else
		t->encap_family = xp->family;

	/* No way to set this via kame pfkey */
	t->allalgs = 1;
	xp->xfrm_nr++;
	return 0;
}