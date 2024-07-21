int ECDSA_verify(int type, const unsigned char *dgst, int dgst_len,
		const unsigned char *sigbuf, int sig_len, EC_KEY *eckey)
 	{
	ECDSA_SIG *s;
	int ret=-1;

	s = ECDSA_SIG_new();
	if (s == NULL) return(ret);
	if (d2i_ECDSA_SIG(&s, &sigbuf, sig_len) == NULL) goto err;
	ret=ECDSA_do_verify(dgst, dgst_len, s, eckey);
err:
	ECDSA_SIG_free(s);
	return(ret);
	}