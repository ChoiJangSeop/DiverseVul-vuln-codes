int ssl_cert_type(X509 *x, EVP_PKEY *pkey)
	{
	EVP_PKEY *pk;
	int ret= -1,i;

	if (pkey == NULL)
		pk=X509_get_pubkey(x);
	else
		pk=pkey;
	if (pk == NULL) goto err;

	i=pk->type;
	if (i == EVP_PKEY_RSA)
		{
		ret=SSL_PKEY_RSA_ENC;
		}
	else if (i == EVP_PKEY_DSA)
		{
		ret=SSL_PKEY_DSA_SIGN;
		}
#ifndef OPENSSL_NO_EC
	else if (i == EVP_PKEY_EC)
		{
		ret = SSL_PKEY_ECC;
		}	
#endif
	else if (i == NID_id_GostR3410_94 || i == NID_id_GostR3410_94_cc) 
		{
		ret = SSL_PKEY_GOST94;
		}
	else if (i == NID_id_GostR3410_2001 || i == NID_id_GostR3410_2001_cc) 
		{
		ret = SSL_PKEY_GOST01;
		}
err:
	if(!pkey) EVP_PKEY_free(pk);
	return(ret);
	}