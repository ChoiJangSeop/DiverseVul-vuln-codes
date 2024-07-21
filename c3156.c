crypto_bob2(
	struct exten *ep,	/* extension pointer */
	struct value *vp	/* value pointer */
	)
{
	RSA	*rsa;		/* GQ parameters */
	DSA_SIG	*sdsa;		/* DSA parameters */
	BN_CTX	*bctx;		/* BIGNUM context */
	EVP_MD_CTX ctx;		/* signature context */
	tstamp_t tstamp;	/* NTP timestamp */
	BIGNUM	*r, *k, *g, *y;
	u_char	*ptr;
	u_int	len;
	int	s_len;

	/*
	 * If the GQ parameters are not valid, something awful
	 * happened or we are being tormented.
	 */
	if (gqkey_info == NULL) {
		msyslog(LOG_NOTICE, "crypto_bob2: scheme unavailable");
		return (XEVNT_ID);
	}
	rsa = gqkey_info->pkey->pkey.rsa;

	/*
	 * Extract r from the challenge.
	 */
	len = ntohl(ep->vallen);
	if ((r = BN_bin2bn((u_char *)ep->pkt, len, NULL)) == NULL) {
		msyslog(LOG_ERR, "crypto_bob2: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		return (XEVNT_ERR);
	}

	/*
	 * Bob rolls random k (0 < k < n), computes y = k u^r mod n and
	 * x = k^b mod n, then sends (y, hash(x)) to Alice. 
	 */
	bctx = BN_CTX_new(); k = BN_new(); g = BN_new(); y = BN_new();
	sdsa = DSA_SIG_new();
	BN_rand(k, len * 8, -1, 1);		/* k */
	BN_mod(k, k, rsa->n, bctx);
	BN_mod_exp(y, rsa->p, r, rsa->n, bctx); /* u^r mod n */
	BN_mod_mul(y, k, y, rsa->n, bctx);	/* k u^r mod n */
	sdsa->r = BN_dup(y);
	BN_mod_exp(g, k, rsa->e, rsa->n, bctx); /* k^b mod n */
	bighash(g, g);
	sdsa->s = BN_dup(g);
	BN_CTX_free(bctx);
	BN_free(r); BN_free(k); BN_free(g); BN_free(y);
#ifdef DEBUG
	if (debug > 1)
		RSA_print_fp(stdout, rsa, 0);
#endif
 
	/*
	 * Encode the values in ASN.1 and sign. The filestamp is from
	 * the local file.
	 */
	len = s_len = i2d_DSA_SIG(sdsa, NULL);
	if (s_len <= 0) {
		msyslog(LOG_ERR, "crypto_bob2: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		DSA_SIG_free(sdsa);
		return (XEVNT_ERR);
	}
	memset(vp, 0, sizeof(struct value));
	tstamp = crypto_time();
	vp->tstamp = htonl(tstamp);
	vp->fstamp = htonl(gqkey_info->fstamp);
	vp->vallen = htonl(len);
	ptr = emalloc(len);
	vp->ptr = ptr;
	i2d_DSA_SIG(sdsa, &ptr);
	DSA_SIG_free(sdsa);
	if (tstamp == 0)
		return (XEVNT_OK);

	vp->sig = emalloc(sign_siglen);
	EVP_SignInit(&ctx, sign_digest);
	EVP_SignUpdate(&ctx, (u_char *)&vp->tstamp, 12);
	EVP_SignUpdate(&ctx, vp->ptr, len);
	if (EVP_SignFinal(&ctx, vp->sig, &len, sign_pkey)) {
		INSIST(len <= sign_siglen);
		vp->siglen = htonl(len);
	}
	return (XEVNT_OK);
}