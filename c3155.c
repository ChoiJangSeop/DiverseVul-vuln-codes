cert_sign(
	struct exten *ep,	/* extension field pointer */
	struct value *vp	/* value pointer */
	)
{
	X509	*req;		/* X509 certificate request */
	X509	*cert;		/* X509 certificate */
	X509_EXTENSION *ext;	/* certificate extension */
	ASN1_INTEGER *serial;	/* serial number */
	X509_NAME *subj;	/* distinguished (common) name */
	EVP_PKEY *pkey;		/* public key */
	EVP_MD_CTX ctx;		/* message digest context */
	tstamp_t tstamp;	/* NTP timestamp */
	struct calendar tscal;
	u_int	len;
	const u_char *cptr;
	u_char *ptr;
	int	i, temp;

	/*
	 * Decode ASN.1 objects and construct certificate structure.
	 * Make sure the system clock is synchronized to a proventic
	 * source.
	 */
	tstamp = crypto_time();
	if (tstamp == 0)
		return (XEVNT_TSP);

	cptr = (void *)ep->pkt;
	if ((req = d2i_X509(NULL, &cptr, ntohl(ep->vallen))) == NULL) {
		msyslog(LOG_ERR, "cert_sign: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		return (XEVNT_CRT);
	}
	/*
	 * Extract public key and check for errors.
	 */
	if ((pkey = X509_get_pubkey(req)) == NULL) {
		msyslog(LOG_ERR, "cert_sign: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		X509_free(req);
		return (XEVNT_PUB);
	}

	/*
	 * Generate X509 certificate signed by this server. If this is a
	 * trusted host, the issuer name is the group name; otherwise,
	 * it is the host name. Also copy any extensions that might be
	 * present.
	 */
	cert = X509_new();
	X509_set_version(cert, X509_get_version(req));
	serial = ASN1_INTEGER_new();
	ASN1_INTEGER_set(serial, tstamp);
	X509_set_serialNumber(cert, serial);
	X509_gmtime_adj(X509_get_notBefore(cert), 0L);
	X509_gmtime_adj(X509_get_notAfter(cert), YEAR);
	subj = X509_get_issuer_name(cert);
	X509_NAME_add_entry_by_txt(subj, "commonName", MBSTRING_ASC,
	    hostval.ptr, strlen((const char *)hostval.ptr), -1, 0);
	subj = X509_get_subject_name(req);
	X509_set_subject_name(cert, subj);
	X509_set_pubkey(cert, pkey);
	temp = X509_get_ext_count(req);
	for (i = 0; i < temp; i++) {
		ext = X509_get_ext(req, i);
		INSIST(X509_add_ext(cert, ext, -1));
	}
	X509_free(req);

	/*
	 * Sign and verify the client certificate, but only if the host
	 * certificate has not expired.
	 */
	(void)ntpcal_ntp_to_date(&tscal, tstamp, NULL);
	if ((calcomp(&tscal, &(cert_host->first)) < 0)
	|| (calcomp(&tscal, &(cert_host->last)) > 0)) {
		X509_free(cert);
		return (XEVNT_PER);
	}
	X509_sign(cert, sign_pkey, sign_digest);
	if (X509_verify(cert, sign_pkey) <= 0) {
		msyslog(LOG_ERR, "cert_sign: %s",
		    ERR_error_string(ERR_get_error(), NULL));
		X509_free(cert);
		return (XEVNT_VFY);
	}
	len = i2d_X509(cert, NULL);

	/*
	 * Build and sign the value structure. We have to sign it here,
	 * since the response has to be returned right away. This is a
	 * clogging hazard.
	 */
	memset(vp, 0, sizeof(struct value));
	vp->tstamp = htonl(tstamp);
	vp->fstamp = ep->fstamp;
	vp->vallen = htonl(len);
	vp->ptr = emalloc(len);
	ptr = vp->ptr;
	i2d_X509(cert, (unsigned char **)(intptr_t)&ptr);
	vp->siglen = 0;
	if (tstamp != 0) {
		vp->sig = emalloc(sign_siglen);
		EVP_SignInit(&ctx, sign_digest);
		EVP_SignUpdate(&ctx, (u_char *)vp, 12);
		EVP_SignUpdate(&ctx, vp->ptr, len);
		if (EVP_SignFinal(&ctx, vp->sig, &len, sign_pkey)) {
			INSIST(len <= sign_siglen);
			vp->siglen = htonl(len);
		}
	}
#ifdef DEBUG
	if (debug > 1)
		X509_print_fp(stdout, cert);
#endif
	X509_free(cert);
	return (XEVNT_OK);
}