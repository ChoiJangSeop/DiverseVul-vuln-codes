static int ssl_set_pkey(CERT *c, EVP_PKEY *pkey)
	{
	int i;

	i=ssl_cert_type(NULL,pkey);
	if (i < 0)
		{
		SSLerr(SSL_F_SSL_SET_PKEY,SSL_R_UNKNOWN_CERTIFICATE_TYPE);
		return(0);
		}

	if (c->pkeys[i].x509 != NULL)
		{
		EVP_PKEY *pktmp;
		pktmp =	X509_get_pubkey(c->pkeys[i].x509);
		EVP_PKEY_copy_parameters(pktmp,pkey);
		EVP_PKEY_free(pktmp);
		ERR_clear_error();

#ifndef OPENSSL_NO_RSA
		/* Don't check the public/private key, this is mostly
		 * for smart cards. */
		if ((pkey->type == EVP_PKEY_RSA) &&
			(RSA_flags(pkey->pkey.rsa) & RSA_METHOD_FLAG_NO_CHECK))
			;
		else
#endif
		if (!X509_check_private_key(c->pkeys[i].x509,pkey))
			{
			X509_free(c->pkeys[i].x509);
			c->pkeys[i].x509 = NULL;
			return 0;
			}
		}

	if (c->pkeys[i].privatekey != NULL)
		EVP_PKEY_free(c->pkeys[i].privatekey);
	CRYPTO_add(&pkey->references,1,CRYPTO_LOCK_EVP_PKEY);
	c->pkeys[i].privatekey=pkey;
	c->key= &(c->pkeys[i]);

	c->valid=0;
	return(1);
	}