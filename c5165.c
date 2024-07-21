static int ipsecrequests_to_migrate(struct sadb_x_ipsecrequest *rq1, int len,
				    struct xfrm_migrate *m)
{
	int err;
	struct sadb_x_ipsecrequest *rq2;
	int mode;

	if (len <= sizeof(struct sadb_x_ipsecrequest) ||
	    len < rq1->sadb_x_ipsecrequest_len)
		return -EINVAL;

	/* old endoints */
	err = parse_sockaddr_pair((struct sockaddr *)(rq1 + 1),
				  rq1->sadb_x_ipsecrequest_len,
				  &m->old_saddr, &m->old_daddr,
				  &m->old_family);
	if (err)
		return err;

	rq2 = (struct sadb_x_ipsecrequest *)((u8 *)rq1 + rq1->sadb_x_ipsecrequest_len);
	len -= rq1->sadb_x_ipsecrequest_len;

	if (len <= sizeof(struct sadb_x_ipsecrequest) ||
	    len < rq2->sadb_x_ipsecrequest_len)
		return -EINVAL;

	/* new endpoints */
	err = parse_sockaddr_pair((struct sockaddr *)(rq2 + 1),
				  rq2->sadb_x_ipsecrequest_len,
				  &m->new_saddr, &m->new_daddr,
				  &m->new_family);
	if (err)
		return err;

	if (rq1->sadb_x_ipsecrequest_proto != rq2->sadb_x_ipsecrequest_proto ||
	    rq1->sadb_x_ipsecrequest_mode != rq2->sadb_x_ipsecrequest_mode ||
	    rq1->sadb_x_ipsecrequest_reqid != rq2->sadb_x_ipsecrequest_reqid)
		return -EINVAL;

	m->proto = rq1->sadb_x_ipsecrequest_proto;
	if ((mode = pfkey_mode_to_xfrm(rq1->sadb_x_ipsecrequest_mode)) < 0)
		return -EINVAL;
	m->mode = mode;
	m->reqid = rq1->sadb_x_ipsecrequest_reqid;

	return ((int)(rq1->sadb_x_ipsecrequest_len +
		      rq2->sadb_x_ipsecrequest_len));
}