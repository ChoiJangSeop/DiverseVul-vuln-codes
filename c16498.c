int DSA_verify(int type, const unsigned char *dgst, int dgst_len,
	     const unsigned char *sigbuf, int siglen, DSA *dsa)
	{
	DSA_SIG *s;
	int ret=-1;
#ifdef OPENSSL_FIPS
	if(FIPS_mode() && !(dsa->flags & DSA_FLAG_NON_FIPS_ALLOW))
		{
		DSAerr(DSA_F_DSA_VERIFY, DSA_R_OPERATION_NOT_ALLOWED_IN_FIPS_MODE);
		return 0;
		}
#endif

	s = DSA_SIG_new();
	if (s == NULL) return(ret);
	if (d2i_DSA_SIG(&s,&sigbuf,siglen) == NULL) goto err;
	ret=DSA_do_verify(dgst,dgst_len,s,dsa);
err:
	DSA_SIG_free(s);
	return(ret);
	}