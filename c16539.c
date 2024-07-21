int ssl_print_sigalgs(BIO *out, SSL *s)
	{
	int i, nsig;
	nsig = SSL_get_sigalgs(s, -1, NULL, NULL, NULL, NULL, NULL);
	if (nsig == 0)
		return 1;

	BIO_puts(out, "Signature Algorithms: ");
	for (i = 0; i < nsig; i++)
		{
		int hash_nid, sign_nid;
		unsigned char rhash, rsign;
		const char *sstr = NULL;
		SSL_get_sigalgs(s, i, &sign_nid, &hash_nid, NULL,
							&rsign, &rhash);
		if (i)
			BIO_puts(out, ":");
		if (sign_nid == EVP_PKEY_RSA)
			sstr = "RSA";
		else if(sign_nid == EVP_PKEY_DSA)
			sstr = "DSA";
		else if(sign_nid == EVP_PKEY_EC)
			sstr = "ECDSA";
		if (sstr)
			BIO_printf(out,"%s+", sstr);
		else
			BIO_printf(out,"0x%02X+", (int)rsign);
		if (hash_nid != NID_undef)
			BIO_printf(out, "%s", OBJ_nid2sn(hash_nid));
		else
			BIO_printf(out,"0x%02X", (int)rhash);
		}
	BIO_puts(out, "\n");
	return 1;
	}