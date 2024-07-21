int SSL_get_sigalgs(SSL *s, int idx,
			int *psign, int *phash, int *psignandhash,
			unsigned char *rsig, unsigned char *rhash)
	{
	if (s->cert->peer_sigalgs == NULL)
		return 0;
	if (idx >= 0)
		{
		TLS_SIGALGS *psig;
		if (idx >= (int)s->cert->peer_sigalgslen)
			return 0;
		psig = s->cert->peer_sigalgs + idx;
		if (psign)
			*psign = psig->sign_nid;
		if (phash)
			*phash = psig->hash_nid;
		if (psignandhash)
			*psignandhash = psig->signandhash_nid;
		if (rsig)
			*rsig = psig->rsign;
		if (rhash)
			*rhash = psig->rhash;
		}
	return s->cert->peer_sigalgslen;
	}