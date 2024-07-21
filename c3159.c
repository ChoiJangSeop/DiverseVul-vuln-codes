crypto_bob3(
	struct exten *ep,	/* extension pointer */
	struct value *vp	/* value pointer */
	)
{
	DSA	*dsa;		/* MV parameters */
	DSA	*sdsa;		/* DSA signature context fake */
	BN_CTX	*bctx;		/* BIGNUM context */
	EVP_MD_CTX ctx;		/* signature context */
	tstamp_t tstamp;	/* NTP timestamp */
	BIGNUM	*r, *k, *u;
	u_char	*ptr;
	u_int	len;

	/*
	 * If the MV parameters are not valid, something awful
	 * happened or we are being tormented.
	 */
	if (mvkey_info == NULL) {
		msyslog(LOG_NOTICE, "crypto_bob3: scheme unavailable");
		return (XEVNT_ID);
	}
	dsa = mvkey_info->pkey->pkey.dsa;

	/*
	 * Extract r from the challenge.
	 */
	len = ntohl(ep->vallen);
	if ((r = BN_bin2bn((u_char *)ep->pkt, len, NULL)) == NULL) {
		msyslog(LOG_ERR, "crypto_bob3: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		return (XEVNT_ERR);
	}

	/*
	 * Bob rolls random k (0 < k < q), making sure it is not a
	 * factor of q. He then computes y = r A^k and sends (y, gbar^k,
	 * and ghat^k) to Alice.
	 */
	bctx = BN_CTX_new(); k = BN_new(); u = BN_new();
	sdsa = DSA_new();
	sdsa->p = BN_new(); sdsa->q = BN_new(); sdsa->g = BN_new();
	while (1) {
		BN_rand(k, BN_num_bits(dsa->q), 0, 0);
		BN_mod(k, k, dsa->q, bctx);
		BN_gcd(u, k, dsa->q, bctx);
		if (BN_is_one(u))
			break;
	}
	BN_mod_exp(u, dsa->g, k, dsa->p, bctx); /* A^k r */
	BN_mod_mul(sdsa->p, u, r, dsa->p, bctx);
	BN_mod_exp(sdsa->q, dsa->priv_key, k, dsa->p, bctx); /* gbar */
	BN_mod_exp(sdsa->g, dsa->pub_key, k, dsa->p, bctx); /* ghat */
	BN_CTX_free(bctx); BN_free(k); BN_free(r); BN_free(u);
#ifdef DEBUG
	if (debug > 1)
		DSA_print_fp(stdout, sdsa, 0);
#endif

	/*
	 * Encode the values in ASN.1 and sign. The filestamp is from
	 * the local file.
	 */
	memset(vp, 0, sizeof(struct value));
	tstamp = crypto_time();
	vp->tstamp = htonl(tstamp);
	vp->fstamp = htonl(mvkey_info->fstamp);
	len = i2d_DSAparams(sdsa, NULL);
	if (len == 0) {
		msyslog(LOG_ERR, "crypto_bob3: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		DSA_free(sdsa);
		return (XEVNT_ERR);
	}
	vp->vallen = htonl(len);
	ptr = emalloc(len);
	vp->ptr = ptr;
	i2d_DSAparams(sdsa, &ptr);
	DSA_free(sdsa);
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