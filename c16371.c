SSL_CIPHER *ssl3_choose_cipher(SSL *s, STACK_OF(SSL_CIPHER) *clnt,
	     STACK_OF(SSL_CIPHER) *srvr)
	{
	SSL_CIPHER *c,*ret=NULL;
	STACK_OF(SSL_CIPHER) *prio, *allow;
	int i,ii,ok;
#if !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_EC)
	unsigned int j;
	int ec_ok, ec_nid;
	unsigned char ec_search1 = 0, ec_search2 = 0;
#endif
	CERT *cert;
	unsigned long alg_k,alg_a,mask_k,mask_a,emask_k,emask_a;

	/* Let's see which ciphers we can support */
	cert=s->cert;

#if 0
	/* Do not set the compare functions, because this may lead to a
	 * reordering by "id". We want to keep the original ordering.
	 * We may pay a price in performance during sk_SSL_CIPHER_find(),
	 * but would have to pay with the price of sk_SSL_CIPHER_dup().
	 */
	sk_SSL_CIPHER_set_cmp_func(srvr, ssl_cipher_ptr_id_cmp);
	sk_SSL_CIPHER_set_cmp_func(clnt, ssl_cipher_ptr_id_cmp);
#endif

#ifdef CIPHER_DEBUG
	printf("Server has %d from %p:\n", sk_SSL_CIPHER_num(srvr), (void *)srvr);
	for(i=0 ; i < sk_SSL_CIPHER_num(srvr) ; ++i)
		{
		c=sk_SSL_CIPHER_value(srvr,i);
		printf("%p:%s\n",(void *)c,c->name);
		}
	printf("Client sent %d from %p:\n", sk_SSL_CIPHER_num(clnt), (void *)clnt);
	for(i=0 ; i < sk_SSL_CIPHER_num(clnt) ; ++i)
	    {
	    c=sk_SSL_CIPHER_value(clnt,i);
	    printf("%p:%s\n",(void *)c,c->name);
	    }
#endif

	if (s->options & SSL_OP_CIPHER_SERVER_PREFERENCE)
		{
		prio = srvr;
		allow = clnt;
		}
	else
		{
		prio = clnt;
		allow = srvr;
		}

	for (i=0; i<sk_SSL_CIPHER_num(prio); i++)
		{
		c=sk_SSL_CIPHER_value(prio,i);

		ssl_set_cert_masks(cert,c);
		mask_k = cert->mask_k;
		mask_a = cert->mask_a;
		emask_k = cert->export_mask_k;
		emask_a = cert->export_mask_a;
			
#ifdef KSSL_DEBUG
/*		printf("ssl3_choose_cipher %d alg= %lx\n", i,c->algorithms);*/
#endif    /* KSSL_DEBUG */

		alg_k=c->algorithm_mkey;
		alg_a=c->algorithm_auth;

#ifndef OPENSSL_NO_KRB5
		if (alg_k & SSL_kKRB5)
			{
			if ( !kssl_keytab_is_available(s->kssl_ctx) )
			    continue;
			}
#endif /* OPENSSL_NO_KRB5 */
#ifndef OPENSSL_NO_PSK
		/* with PSK there must be server callback set */
		if ((alg_k & SSL_kPSK) && s->psk_server_callback == NULL)
			continue;
#endif /* OPENSSL_NO_PSK */

		if (SSL_C_IS_EXPORT(c))
			{
			ok = (alg_k & emask_k) && (alg_a & emask_a);
#ifdef CIPHER_DEBUG
			printf("%d:[%08lX:%08lX:%08lX:%08lX]%p:%s (export)\n",ok,alg_k,alg_a,emask_k,emask_a,
			       (void *)c,c->name);
#endif
			}
		else
			{
			ok = (alg_k & mask_k) && (alg_a & mask_a);
#ifdef CIPHER_DEBUG
			printf("%d:[%08lX:%08lX:%08lX:%08lX]%p:%s\n",ok,alg_k,alg_a,mask_k,mask_a,(void *)c,
			       c->name);
#endif
			}

#ifndef OPENSSL_NO_TLSEXT
#ifndef OPENSSL_NO_EC
		if (
			/* if we are considering an ECC cipher suite that uses our certificate */
			(alg_a & SSL_aECDSA || alg_a & SSL_aECDH)
			/* and we have an ECC certificate */
			&& (s->cert->pkeys[SSL_PKEY_ECC].x509 != NULL)
			/* and the client specified a Supported Point Formats extension */
			&& ((s->session->tlsext_ecpointformatlist_length > 0) && (s->session->tlsext_ecpointformatlist != NULL))
			/* and our certificate's point is compressed */
			&& (
				(s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info->key != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info->key->public_key != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info->key->public_key->data != NULL)
				&& (
					(*(s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info->key->public_key->data) == POINT_CONVERSION_COMPRESSED)
					|| (*(s->cert->pkeys[SSL_PKEY_ECC].x509->cert_info->key->public_key->data) == POINT_CONVERSION_COMPRESSED + 1)
					)
				)
		)
			{
			ec_ok = 0;
			/* if our certificate's curve is over a field type that the client does not support
			 * then do not allow this cipher suite to be negotiated */
			if (
				(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth != NULL)
				&& (EC_METHOD_get_field_type(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth) == NID_X9_62_prime_field)
			)
				{
				for (j = 0; j < s->session->tlsext_ecpointformatlist_length; j++)
					{
					if (s->session->tlsext_ecpointformatlist[j] == TLSEXT_ECPOINTFORMAT_ansiX962_compressed_prime)
						{
						ec_ok = 1;
						break;
						}
					}
				}
			else if (EC_METHOD_get_field_type(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth) == NID_X9_62_characteristic_two_field)
				{
				for (j = 0; j < s->session->tlsext_ecpointformatlist_length; j++)
					{
					if (s->session->tlsext_ecpointformatlist[j] == TLSEXT_ECPOINTFORMAT_ansiX962_compressed_char2)
						{
						ec_ok = 1;
						break;
						}
					}
				}
			ok = ok && ec_ok;
			}
		if (
			/* if we are considering an ECC cipher suite that uses our certificate */
			(alg_a & SSL_aECDSA || alg_a & SSL_aECDH)
			/* and we have an ECC certificate */
			&& (s->cert->pkeys[SSL_PKEY_ECC].x509 != NULL)
			/* and the client specified an EllipticCurves extension */
			&& ((s->session->tlsext_ellipticcurvelist_length > 0) && (s->session->tlsext_ellipticcurvelist != NULL))
		)
			{
			ec_ok = 0;
			if (
				(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec != NULL)
				&& (s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group != NULL)
			)
				{
				ec_nid = EC_GROUP_get_curve_name(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group);
				if ((ec_nid == 0)
					&& (s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth != NULL)
				)
					{
					if (EC_METHOD_get_field_type(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth) == NID_X9_62_prime_field)
						{
						ec_search1 = 0xFF;
						ec_search2 = 0x01;
						}
					else if (EC_METHOD_get_field_type(s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec->group->meth) == NID_X9_62_characteristic_two_field)
						{
						ec_search1 = 0xFF;
						ec_search2 = 0x02;
						}
					}
				else
					{
					ec_search1 = 0x00;
					ec_search2 = tls1_ec_nid2curve_id(ec_nid);
					}
				if ((ec_search1 != 0) || (ec_search2 != 0))
					{
					for (j = 0; j < s->session->tlsext_ellipticcurvelist_length / 2; j++)
						{
						if ((s->session->tlsext_ellipticcurvelist[2*j] == ec_search1) && (s->session->tlsext_ellipticcurvelist[2*j+1] == ec_search2))
							{
							ec_ok = 1;
							break;
							}
						}
					}
				}
			ok = ok && ec_ok;
			}
		if (
			/* if we are considering an ECC cipher suite that uses an ephemeral EC key */
			(alg_k & SSL_kEECDH)
			/* and we have an ephemeral EC key */
			&& (s->cert->ecdh_tmp != NULL)
			/* and the client specified an EllipticCurves extension */
			&& ((s->session->tlsext_ellipticcurvelist_length > 0) && (s->session->tlsext_ellipticcurvelist != NULL))
		)
			{
			ec_ok = 0;
			if (s->cert->ecdh_tmp->group != NULL)
				{
				ec_nid = EC_GROUP_get_curve_name(s->cert->ecdh_tmp->group);
				if ((ec_nid == 0)
					&& (s->cert->ecdh_tmp->group->meth != NULL)
				)
					{
					if (EC_METHOD_get_field_type(s->cert->ecdh_tmp->group->meth) == NID_X9_62_prime_field)
						{
						ec_search1 = 0xFF;
						ec_search2 = 0x01;
						}
					else if (EC_METHOD_get_field_type(s->cert->ecdh_tmp->group->meth) == NID_X9_62_characteristic_two_field)
						{
						ec_search1 = 0xFF;
						ec_search2 = 0x02;
						}
					}
				else
					{
					ec_search1 = 0x00;
					ec_search2 = tls1_ec_nid2curve_id(ec_nid);
					}
				if ((ec_search1 != 0) || (ec_search2 != 0))
					{
					for (j = 0; j < s->session->tlsext_ellipticcurvelist_length / 2; j++)
						{
						if ((s->session->tlsext_ellipticcurvelist[2*j] == ec_search1) && (s->session->tlsext_ellipticcurvelist[2*j+1] == ec_search2))
							{
							ec_ok = 1;
							break;
							}
						}
					}
				}
			ok = ok && ec_ok;
			}
#endif /* OPENSSL_NO_EC */
#endif /* OPENSSL_NO_TLSEXT */

		if (!ok) continue;
		ii=sk_SSL_CIPHER_find(allow,c);
		if (ii >= 0)
			{
			ret=sk_SSL_CIPHER_value(allow,ii);
			break;
			}
		}
	return(ret);
	}