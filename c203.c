int sctp_auth_ep_set_hmacs(struct sctp_endpoint *ep,
			   struct sctp_hmacalgo *hmacs)
{
	int has_sha1 = 0;
	__u16 id;
	int i;

	/* Scan the list looking for unsupported id.  Also make sure that
	 * SHA1 is specified.
	 */
	for (i = 0; i < hmacs->shmac_num_idents; i++) {
		id = hmacs->shmac_idents[i];

		if (SCTP_AUTH_HMAC_ID_SHA1 == id)
			has_sha1 = 1;

		if (!sctp_hmac_list[id].hmac_name)
			return -EOPNOTSUPP;
	}

	if (!has_sha1)
		return -EINVAL;

	memcpy(ep->auth_hmacs_list->hmac_ids, &hmacs->shmac_idents[0],
		hmacs->shmac_num_idents * sizeof(__u16));
	ep->auth_hmacs_list->param_hdr.length = htons(sizeof(sctp_paramhdr_t) +
				hmacs->shmac_num_idents * sizeof(__u16));
	return 0;
}