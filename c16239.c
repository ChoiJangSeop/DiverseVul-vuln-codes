int OCSP_basic_verify(OCSP_BASICRESP *bs, STACK_OF(X509) *certs,
				X509_STORE *st, unsigned long flags)
	{
	X509 *signer, *x;
	STACK_OF(X509) *chain = NULL;
	X509_STORE_CTX ctx;
	int i, ret = 0;
	ret = ocsp_find_signer(&signer, bs, certs, st, flags);
	if (!ret)
		{
		OCSPerr(OCSP_F_OCSP_BASIC_VERIFY, OCSP_R_SIGNER_CERTIFICATE_NOT_FOUND);
		goto end;
		}
	if ((ret == 2) && (flags & OCSP_TRUSTOTHER))
		flags |= OCSP_NOVERIFY;
	if (!(flags & OCSP_NOSIGS))
		{
		EVP_PKEY *skey;
		skey = X509_get_pubkey(signer);
		ret = OCSP_BASICRESP_verify(bs, skey, 0);
		EVP_PKEY_free(skey);
		if(ret <= 0)
			{
			OCSPerr(OCSP_F_OCSP_BASIC_VERIFY, OCSP_R_SIGNATURE_FAILURE);
			goto end;
			}
		}
	if (!(flags & OCSP_NOVERIFY))
		{
		int init_res;
		if(flags & OCSP_NOCHAIN)
			init_res = X509_STORE_CTX_init(&ctx, st, signer, NULL);
		else
			init_res = X509_STORE_CTX_init(&ctx, st, signer, bs->certs);
		if(!init_res)
			{
			ret = -1;
			OCSPerr(OCSP_F_OCSP_BASIC_VERIFY,ERR_R_X509_LIB);
			goto end;
			}

		X509_STORE_CTX_set_purpose(&ctx, X509_PURPOSE_OCSP_HELPER);
		ret = X509_verify_cert(&ctx);
		chain = X509_STORE_CTX_get1_chain(&ctx);
		X509_STORE_CTX_cleanup(&ctx);
                if (ret <= 0)
			{
			i = X509_STORE_CTX_get_error(&ctx);	
			OCSPerr(OCSP_F_OCSP_BASIC_VERIFY,OCSP_R_CERTIFICATE_VERIFY_ERROR);
			ERR_add_error_data(2, "Verify error:",
					X509_verify_cert_error_string(i));
                        goto end;
                	}
		if(flags & OCSP_NOCHECKS)
			{
			ret = 1;
			goto end;
			}
		/* At this point we have a valid certificate chain
		 * need to verify it against the OCSP issuer criteria.
		 */
		ret = ocsp_check_issuer(bs, chain, flags);

		/* If fatal error or valid match then finish */
		if (ret != 0) goto end;

		/* Easy case: explicitly trusted. Get root CA and
		 * check for explicit trust
		 */
		if(flags & OCSP_NOEXPLICIT) goto end;

		x = sk_X509_value(chain, sk_X509_num(chain) - 1);
		if(X509_check_trust(x, NID_OCSP_sign, 0) != X509_TRUST_TRUSTED)
			{
			OCSPerr(OCSP_F_OCSP_BASIC_VERIFY,OCSP_R_ROOT_CA_NOT_TRUSTED);
			goto end;
			}
		ret = 1;
		}



	end:
	if(chain) sk_X509_pop_free(chain, X509_free);
	return ret;
	}