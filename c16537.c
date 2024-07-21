CERT *ssl_cert_dup(CERT *cert)
	{
	CERT *ret;
	int i;

	ret = (CERT *)OPENSSL_malloc(sizeof(CERT));
	if (ret == NULL)
		{
		SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_MALLOC_FAILURE);
		return(NULL);
		}

	memset(ret, 0, sizeof(CERT));

	ret->key = &ret->pkeys[cert->key - &cert->pkeys[0]];
	/* or ret->key = ret->pkeys + (cert->key - cert->pkeys),
	 * if you find that more readable */

	ret->valid = cert->valid;
	ret->mask_k = cert->mask_k;
	ret->mask_a = cert->mask_a;
	ret->export_mask_k = cert->export_mask_k;
	ret->export_mask_a = cert->export_mask_a;

#ifndef OPENSSL_NO_RSA
	if (cert->rsa_tmp != NULL)
		{
		RSA_up_ref(cert->rsa_tmp);
		ret->rsa_tmp = cert->rsa_tmp;
		}
	ret->rsa_tmp_cb = cert->rsa_tmp_cb;
#endif

#ifndef OPENSSL_NO_DH
	if (cert->dh_tmp != NULL)
		{
		ret->dh_tmp = DHparams_dup(cert->dh_tmp);
		if (ret->dh_tmp == NULL)
			{
			SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_DH_LIB);
			goto err;
			}
		if (cert->dh_tmp->priv_key)
			{
			BIGNUM *b = BN_dup(cert->dh_tmp->priv_key);
			if (!b)
				{
				SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_BN_LIB);
				goto err;
				}
			ret->dh_tmp->priv_key = b;
			}
		if (cert->dh_tmp->pub_key)
			{
			BIGNUM *b = BN_dup(cert->dh_tmp->pub_key);
			if (!b)
				{
				SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_BN_LIB);
				goto err;
				}
			ret->dh_tmp->pub_key = b;
			}
		}
	ret->dh_tmp_cb = cert->dh_tmp_cb;
#endif

#ifndef OPENSSL_NO_ECDH
	if (cert->ecdh_tmp)
		{
		ret->ecdh_tmp = EC_KEY_dup(cert->ecdh_tmp);
		if (ret->ecdh_tmp == NULL)
			{
			SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_EC_LIB);
			goto err;
			}
		}
	ret->ecdh_tmp_cb = cert->ecdh_tmp_cb;
	ret->ecdh_tmp_auto = cert->ecdh_tmp_auto;
#endif

	for (i = 0; i < SSL_PKEY_NUM; i++)
		{
		CERT_PKEY *cpk = cert->pkeys + i;
		CERT_PKEY *rpk = ret->pkeys + i;
		if (cpk->x509 != NULL)
			{
			rpk->x509 = cpk->x509;
			CRYPTO_add(&rpk->x509->references, 1, CRYPTO_LOCK_X509);
			}
		
		if (cpk->privatekey != NULL)
			{
			rpk->privatekey = cpk->privatekey;
			CRYPTO_add(&cpk->privatekey->references, 1,
				CRYPTO_LOCK_EVP_PKEY);

			switch(i) 
				{
				/* If there was anything special to do for
				 * certain types of keys, we'd do it here.
				 * (Nothing at the moment, I think.) */

			case SSL_PKEY_RSA_ENC:
			case SSL_PKEY_RSA_SIGN:
				/* We have an RSA key. */
				break;
				
			case SSL_PKEY_DSA_SIGN:
				/* We have a DSA key. */
				break;
				
			case SSL_PKEY_DH_RSA:
			case SSL_PKEY_DH_DSA:
				/* We have a DH key. */
				break;

			case SSL_PKEY_ECC:
				/* We have an ECC key */
				break;

			default:
				/* Can't happen. */
				SSLerr(SSL_F_SSL_CERT_DUP, SSL_R_LIBRARY_BUG);
				}
			}

		if (cpk->chain)
			{
			int j;
			rpk->chain = sk_X509_dup(cpk->chain);
			if (!rpk->chain)
				{
				SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_MALLOC_FAILURE);
				goto err;
				}
			for (j = 0; j < sk_X509_num(rpk->chain); j++)
				{
				X509 *x = sk_X509_value(rpk->chain, j);
				CRYPTO_add(&x->references, 1, CRYPTO_LOCK_X509);
				}
			}
                if (cert->pkeys[i].authz != NULL)
			{
			/* Just copy everything. */
			ret->pkeys[i].authz_length =
				cert->pkeys[i].authz_length;
			ret->pkeys[i].authz =
				OPENSSL_malloc(ret->pkeys[i].authz_length);
			if (ret->pkeys[i].authz == NULL)
				{
				SSLerr(SSL_F_SSL_CERT_DUP, ERR_R_MALLOC_FAILURE);
				return(NULL);
				}
			memcpy(ret->pkeys[i].authz,
			       cert->pkeys[i].authz,
			       cert->pkeys[i].authz_length);
			}
		}
	
	ret->references=1;
	/* Set digests to defaults. NB: we don't copy existing values as they
	 * will be set during handshake.
	 */
	ssl_cert_set_default_md(ret);
	/* Peer sigalgs set to NULL as we get these from handshake too */
	ret->peer_sigalgs = NULL;
	ret->peer_sigalgslen = 0;
	/* Configure sigalgs however we copy across */
	if (cert->conf_sigalgs)
		{
		ret->conf_sigalgs = OPENSSL_malloc(cert->conf_sigalgslen
							* sizeof(TLS_SIGALGS));
		if (!ret->conf_sigalgs)
			goto err;
		memcpy(ret->conf_sigalgs, cert->conf_sigalgs,
				cert->conf_sigalgslen * sizeof(TLS_SIGALGS));
		ret->conf_sigalgslen = cert->conf_sigalgslen;
		}
	else
		ret->conf_sigalgs = NULL;

	return(ret);
	
#if !defined(OPENSSL_NO_DH) || !defined(OPENSSL_NO_ECDH)
err:
#endif
#ifndef OPENSSL_NO_RSA
	if (ret->rsa_tmp != NULL)
		RSA_free(ret->rsa_tmp);
#endif
#ifndef OPENSSL_NO_DH
	if (ret->dh_tmp != NULL)
		DH_free(ret->dh_tmp);
#endif
#ifndef OPENSSL_NO_ECDH
	if (ret->ecdh_tmp != NULL)
		EC_KEY_free(ret->ecdh_tmp);
#endif

	ssl_cert_clear_certs(ret);

	return NULL;
	}