int tls1_set_sigalgs(CERT *c, const int *salg, size_t salglen)
	{
	TLS_SIGALGS *sigalgs, *sptr;
	int rhash, rsign;
	size_t i;
	if (salglen & 1)
		return 0;
	salglen /= 2;
	sigalgs = OPENSSL_malloc(sizeof(TLS_SIGALGS) * salglen);
	if (sigalgs == NULL)
		return 0;
	for (i = 0, sptr = sigalgs; i < salglen; i++, sptr++)
		{
		sptr->hash_nid = *salg++;
		sptr->sign_nid = *salg++;
		rhash = tls12_find_id(sptr->hash_nid, tls12_md,
					sizeof(tls12_md)/sizeof(tls12_lookup));
		rsign = tls12_find_id(sptr->sign_nid, tls12_sig,
				sizeof(tls12_sig)/sizeof(tls12_lookup));

		if (rhash == -1 || rsign == -1)
			goto err;

		if (!OBJ_find_sigid_by_algs(&sptr->signandhash_nid,
						sptr->hash_nid,
						sptr->sign_nid))
			sptr->signandhash_nid = NID_undef;
		sptr->rhash = rhash;
		sptr->rsign = rsign;
		}

	if (c->conf_sigalgs)
		OPENSSL_free(c->conf_sigalgs);

	c->conf_sigalgs = sigalgs;
	c->conf_sigalgslen = salglen;
	return 1;

	err:
	OPENSSL_free(sigalgs);
	return 0;
	}