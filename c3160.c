crypto_bob(
	struct exten *ep,	/* extension pointer */
	struct value *vp	/* value pointer */
	)
{
	DSA	*dsa;		/* IFF parameters */
	DSA_SIG	*sdsa;		/* DSA signature context fake */
	BN_CTX	*bctx;		/* BIGNUM context */
	EVP_MD_CTX ctx;		/* signature context */
	tstamp_t tstamp;	/* NTP timestamp */
	BIGNUM	*bn, *bk, *r;
	u_char	*ptr;
	u_int	len;		/* extension field length */
	u_int	vallen = 0;	/* value length */

	/*
	 * If the IFF parameters are not valid, something awful
	 * happened or we are being tormented.
	 */
	if (iffkey_info == NULL) {
		msyslog(LOG_NOTICE, "crypto_bob: scheme unavailable");
		return (XEVNT_ID);
	}
	dsa = iffkey_info->pkey->pkey.dsa;

	/*
	 * Extract r from the challenge.
	 */
	vallen = ntohl(ep->vallen);
	len = ntohl(ep->opcode) & 0x0000ffff;
	if (vallen == 0 || len < VALUE_LEN || len - VALUE_LEN < vallen)
		return XEVNT_LEN;
	if ((r = BN_bin2bn((u_char *)ep->pkt, vallen, NULL)) == NULL) {
		msyslog(LOG_ERR, "crypto_bob: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		return (XEVNT_ERR);
	}

	/*
	 * Bob rolls random k (0 < k < q), computes y = k + b r mod q
	 * and x = g^k mod p, then sends (y, hash(x)) to Alice.
	 */
	bctx = BN_CTX_new(); bk = BN_new(); bn = BN_new();
	sdsa = DSA_SIG_new();
	BN_rand(bk, vallen * 8, -1, 1);		/* k */
	BN_mod_mul(bn, dsa->priv_key, r, dsa->q, bctx); /* b r mod q */
	BN_add(bn, bn, bk);
	BN_mod(bn, bn, dsa->q, bctx);		/* k + b r mod q */
	sdsa->r = BN_dup(bn);
	BN_mod_exp(bk, dsa->g, bk, dsa->p, bctx); /* g^k mod p */
	bighash(bk, bk);
	sdsa->s = BN_dup(bk);
	BN_CTX_free(bctx);
	BN_free(r); BN_free(bn); BN_free(bk);
#ifdef DEBUG
	if (debug > 1)
		DSA_print_fp(stdout, dsa, 0);
#endif

	/*
	 * Encode the values in ASN.1 and sign. The filestamp is from
	 * the local file.
	 */
	vallen = i2d_DSA_SIG(sdsa, NULL);
	if (vallen == 0) {
		msyslog(LOG_ERR, "crypto_bob: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		DSA_SIG_free(sdsa);
		return (XEVNT_ERR);
	}
	if (vallen > MAX_VALLEN) {
		msyslog(LOG_ERR, "crypto_bob: signature is too big: %d",
		    vallen);
		DSA_SIG_free(sdsa);
		return (XEVNT_LEN);
	}
	memset(vp, 0, sizeof(struct value));
	tstamp = crypto_time();
	vp->tstamp = htonl(tstamp);
	vp->fstamp = htonl(iffkey_info->fstamp);
	vp->vallen = htonl(vallen);
	ptr = emalloc(vallen);
	vp->ptr = ptr;
	i2d_DSA_SIG(sdsa, &ptr);
	DSA_SIG_free(sdsa);
	if (tstamp == 0)
		return (XEVNT_OK);

	/* XXX: more validation to make sure the sign fits... */
	vp->sig = emalloc(sign_siglen);
	EVP_SignInit(&ctx, sign_digest);
	EVP_SignUpdate(&ctx, (u_char *)&vp->tstamp, 12);
	EVP_SignUpdate(&ctx, vp->ptr, vallen);
	if (EVP_SignFinal(&ctx, vp->sig, &vallen, sign_pkey)) {
		INSIST(vallen <= sign_siglen);
		vp->siglen = htonl(vallen);
	}
	return (XEVNT_OK);
}