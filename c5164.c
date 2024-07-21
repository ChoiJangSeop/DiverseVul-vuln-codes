parse_ipsecrequests(struct xfrm_policy *xp, struct sadb_x_policy *pol)
{
	int err;
	int len = pol->sadb_x_policy_len*8 - sizeof(struct sadb_x_policy);
	struct sadb_x_ipsecrequest *rq = (void*)(pol+1);

	if (pol->sadb_x_policy_len * 8 < sizeof(struct sadb_x_policy))
		return -EINVAL;

	while (len >= sizeof(struct sadb_x_ipsecrequest)) {
		if ((err = parse_ipsecrequest(xp, rq)) < 0)
			return err;
		len -= rq->sadb_x_ipsecrequest_len;
		rq = (void*)((u8*)rq + rq->sadb_x_ipsecrequest_len);
	}
	return 0;
}