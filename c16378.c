int ssl3_get_client_key_exchange(SSL *s)
	{
	int i,al,ok;
	long n;
	unsigned long alg_k;
	unsigned char *p;
#ifndef OPENSSL_NO_RSA
	RSA *rsa=NULL;
	EVP_PKEY *pkey=NULL;
#endif
#ifndef OPENSSL_NO_DH
	BIGNUM *pub=NULL;
	DH *dh_srvr;
#endif
#ifndef OPENSSL_NO_KRB5
	KSSL_ERR kssl_err;
#endif /* OPENSSL_NO_KRB5 */

#ifndef OPENSSL_NO_ECDH
	EC_KEY *srvr_ecdh = NULL;
	EVP_PKEY *clnt_pub_pkey = NULL;
	EC_POINT *clnt_ecpoint = NULL;
	BN_CTX *bn_ctx = NULL; 
#endif

	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_KEY_EXCH_A,
		SSL3_ST_SR_KEY_EXCH_B,
		SSL3_MT_CLIENT_KEY_EXCHANGE,
		2048, /* ??? */
		&ok);

	if (!ok) return((int)n);
	p=(unsigned char *)s->init_msg;

	alg_k=s->s3->tmp.new_cipher->algorithm_mkey;

#ifndef OPENSSL_NO_RSA
	if (alg_k & SSL_kRSA)
		{
		/* FIX THIS UP EAY EAY EAY EAY */
		if (s->s3->tmp.use_rsa_tmp)
			{
			if ((s->cert != NULL) && (s->cert->rsa_tmp != NULL))
				rsa=s->cert->rsa_tmp;
			/* Don't do a callback because rsa_tmp should
			 * be sent already */
			if (rsa == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_RSA_PKEY);
				goto f_err;

				}
			}
		else
			{
			pkey=s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey;
			if (	(pkey == NULL) ||
				(pkey->type != EVP_PKEY_RSA) ||
				(pkey->pkey.rsa == NULL))
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_RSA_CERTIFICATE);
				goto f_err;
				}
			rsa=pkey->pkey.rsa;
			}

		/* TLS and [incidentally] DTLS{0xFEFF} */
		if (s->version > SSL3_VERSION && s->version != DTLS1_BAD_VER)
			{
			n2s(p,i);
			if (n != i+2)
				{
				if (!(s->options & SSL_OP_TLS_D5_BUG))
					{
					SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_TLS_RSA_ENCRYPTED_VALUE_LENGTH_IS_WRONG);
					goto err;
					}
				else
					p-=2;
				}
			else
				n=i;
			}

		i=RSA_private_decrypt((int)n,p,p,rsa,RSA_PKCS1_PADDING);

		al = -1;
		
		if (i != SSL_MAX_MASTER_KEY_LENGTH)
			{
			al=SSL_AD_DECODE_ERROR;
			/* SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BAD_RSA_DECRYPT); */
			}

		if ((al == -1) && !((p[0] == (s->client_version>>8)) && (p[1] == (s->client_version & 0xff))))
			{
			/* The premaster secret must contain the same version number as the
			 * ClientHello to detect version rollback attacks (strangely, the
			 * protocol does not offer such protection for DH ciphersuites).
			 * However, buggy clients exist that send the negotiated protocol
			 * version instead if the server does not support the requested
			 * protocol version.
			 * If SSL_OP_TLS_ROLLBACK_BUG is set, tolerate such clients. */
			if (!((s->options & SSL_OP_TLS_ROLLBACK_BUG) &&
				(p[0] == (s->version>>8)) && (p[1] == (s->version & 0xff))))
				{
				al=SSL_AD_DECODE_ERROR;
				/* SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BAD_PROTOCOL_VERSION_NUMBER); */

				/* The Klima-Pokorny-Rosa extension of Bleichenbacher's attack
				 * (http://eprint.iacr.org/2003/052/) exploits the version
				 * number check as a "bad version oracle" -- an alert would
				 * reveal that the plaintext corresponding to some ciphertext
				 * made up by the adversary is properly formatted except
				 * that the version number is wrong.  To avoid such attacks,
				 * we should treat this just like any other decryption error. */
				}
			}

		if (al != -1)
			{
			/* Some decryption failure -- use random value instead as countermeasure
			 * against Bleichenbacher's attack on PKCS #1 v1.5 RSA padding
			 * (see RFC 2246, section 7.4.7.1). */
			ERR_clear_error();
			i = SSL_MAX_MASTER_KEY_LENGTH;
			p[0] = s->client_version >> 8;
			p[1] = s->client_version & 0xff;
			if (RAND_pseudo_bytes(p+2, i-2) <= 0) /* should be RAND_bytes, but we cannot work around a failure */
				goto err;
			}
	
		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key,
				p,i);
		OPENSSL_cleanse(p,i);
		}
	else
#endif
#ifndef OPENSSL_NO_DH
		if (alg_k & (SSL_kEDH|SSL_kDHr|SSL_kDHd))
		{
		n2s(p,i);
		if (n != i+2)
			{
			if (!(s->options & SSL_OP_SSLEAY_080_CLIENT_DH_BUG))
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DH_PUBLIC_VALUE_LENGTH_IS_WRONG);
				goto err;
				}
			else
				{
				p-=2;
				i=(int)n;
				}
			}

		if (n == 0L) /* the parameters are in the cert */
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_UNABLE_TO_DECODE_DH_CERTS);
			goto f_err;
			}
		else
			{
			if (s->s3->tmp.dh == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_DH_KEY);
				goto f_err;
				}
			else
				dh_srvr=s->s3->tmp.dh;
			}

		pub=BN_bin2bn(p,i,NULL);
		if (pub == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BN_LIB);
			goto err;
			}

		i=DH_compute_key(p,pub,dh_srvr);

		if (i <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,ERR_R_DH_LIB);
			goto err;
			}

		DH_free(s->s3->tmp.dh);
		s->s3->tmp.dh=NULL;

		BN_clear_free(pub);
		pub=NULL;
		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key,p,i);
		OPENSSL_cleanse(p,i);
		}
	else
#endif
#ifndef OPENSSL_NO_KRB5
	if (alg_k & SSL_kKRB5)
		{
		krb5_error_code		krb5rc;
		krb5_data		enc_ticket;
		krb5_data		authenticator;
		krb5_data		enc_pms;
		KSSL_CTX		*kssl_ctx = s->kssl_ctx;
		EVP_CIPHER_CTX		ciph_ctx;
		const EVP_CIPHER	*enc = NULL;
		unsigned char		iv[EVP_MAX_IV_LENGTH];
		unsigned char		pms[SSL_MAX_MASTER_KEY_LENGTH
					       + EVP_MAX_BLOCK_LENGTH];
		int		     padl, outl;
		krb5_timestamp		authtime = 0;
		krb5_ticket_times	ttimes;

		EVP_CIPHER_CTX_init(&ciph_ctx);

		if (!kssl_ctx)  kssl_ctx = kssl_ctx_new();

		n2s(p,i);
		enc_ticket.length = i;

		if (n < (long)(enc_ticket.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		enc_ticket.data = (char *)p;
		p+=enc_ticket.length;

		n2s(p,i);
		authenticator.length = i;

		if (n < (long)(enc_ticket.length + authenticator.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		authenticator.data = (char *)p;
		p+=authenticator.length;

		n2s(p,i);
		enc_pms.length = i;
		enc_pms.data = (char *)p;
		p+=enc_pms.length;

		/* Note that the length is checked again below,
		** after decryption
		*/
		if(enc_pms.length > sizeof pms)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		if (n != (long)(enc_ticket.length + authenticator.length +
						enc_pms.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		if ((krb5rc = kssl_sget_tkt(kssl_ctx, &enc_ticket, &ttimes,
					&kssl_err)) != 0)
			{
#ifdef KSSL_DEBUG
			printf("kssl_sget_tkt rtn %d [%d]\n",
				krb5rc, kssl_err.reason);
			if (kssl_err.text)
				printf("kssl_err text= %s\n", kssl_err.text);
#endif	/* KSSL_DEBUG */
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				kssl_err.reason);
			goto err;
			}

		/*  Note: no authenticator is not considered an error,
		**  but will return authtime == 0.
		*/
		if ((krb5rc = kssl_check_authent(kssl_ctx, &authenticator,
					&authtime, &kssl_err)) != 0)
			{
#ifdef KSSL_DEBUG
			printf("kssl_check_authent rtn %d [%d]\n",
				krb5rc, kssl_err.reason);
			if (kssl_err.text)
				printf("kssl_err text= %s\n", kssl_err.text);
#endif	/* KSSL_DEBUG */
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				kssl_err.reason);
			goto err;
			}

		if ((krb5rc = kssl_validate_times(authtime, &ttimes)) != 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE, krb5rc);
			goto err;
			}

#ifdef KSSL_DEBUG
		kssl_ctx_show(kssl_ctx);
#endif	/* KSSL_DEBUG */

		enc = kssl_map_enc(kssl_ctx->enctype);
		if (enc == NULL)
		    goto err;

		memset(iv, 0, sizeof iv);	/* per RFC 1510 */

		if (!EVP_DecryptInit_ex(&ciph_ctx,enc,NULL,kssl_ctx->key,iv))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		if (!EVP_DecryptUpdate(&ciph_ctx, pms,&outl,
					(unsigned char *)enc_pms.data, enc_pms.length))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		if (outl > SSL_MAX_MASTER_KEY_LENGTH)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}
		if (!EVP_DecryptFinal_ex(&ciph_ctx,&(pms[outl]),&padl))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		outl += padl;
		if (outl > SSL_MAX_MASTER_KEY_LENGTH)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}
		if (!((pms[0] == (s->client_version>>8)) && (pms[1] == (s->client_version & 0xff))))
		    {
		    /* The premaster secret must contain the same version number as the
		     * ClientHello to detect version rollback attacks (strangely, the
		     * protocol does not offer such protection for DH ciphersuites).
		     * However, buggy clients exist that send random bytes instead of
		     * the protocol version.
		     * If SSL_OP_TLS_ROLLBACK_BUG is set, tolerate such clients. 
		     * (Perhaps we should have a separate BUG value for the Kerberos cipher)
		     */
		    if (!(s->options & SSL_OP_TLS_ROLLBACK_BUG))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       SSL_AD_DECODE_ERROR);
			goto err;
			}
		    }

		EVP_CIPHER_CTX_cleanup(&ciph_ctx);

		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key, pms, outl);

		if (kssl_ctx->client_princ)
			{
			size_t len = strlen(kssl_ctx->client_princ);
			if ( len < SSL_MAX_KRB5_PRINCIPAL_LENGTH ) 
				{
				s->session->krb5_client_princ_len = len;
				memcpy(s->session->krb5_client_princ,kssl_ctx->client_princ,len);
				}
			}


		/*  Was doing kssl_ctx_free() here,
		**  but it caused problems for apache.
		**  kssl_ctx = kssl_ctx_free(kssl_ctx);
		**  if (s->kssl_ctx)  s->kssl_ctx = NULL;
		*/
		}
	else
#endif	/* OPENSSL_NO_KRB5 */

#ifndef OPENSSL_NO_ECDH
		if (alg_k & (SSL_kEECDH|SSL_kECDHr|SSL_kECDHe))
		{
		int ret = 1;
		int field_size = 0;
		const EC_KEY   *tkey;
		const EC_GROUP *group;
		const BIGNUM *priv_key;

		/* initialize structures for server's ECDH key pair */
		if ((srvr_ecdh = EC_KEY_new()) == NULL) 
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_MALLOC_FAILURE);
			goto err;
			}

		/* Let's get server private key and group information */
		if (alg_k & (SSL_kECDHr|SSL_kECDHe))
			{ 
			/* use the certificate */
			tkey = s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec;
			}
		else
			{
			/* use the ephermeral values we saved when
			 * generating the ServerKeyExchange msg.
			 */
			tkey = s->s3->tmp.ecdh;
			}

		group    = EC_KEY_get0_group(tkey);
		priv_key = EC_KEY_get0_private_key(tkey);

		if (!EC_KEY_set_group(srvr_ecdh, group) ||
		    !EC_KEY_set_private_key(srvr_ecdh, priv_key))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       ERR_R_EC_LIB);
			goto err;
			}

		/* Let's get client's public key */
		if ((clnt_ecpoint = EC_POINT_new(group)) == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_MALLOC_FAILURE);
			goto err;
			}

		if (n == 0L) 
			{
			/* Client Publickey was in Client Certificate */

			 if (alg_k & SSL_kEECDH)
				 {
				 al=SSL_AD_HANDSHAKE_FAILURE;
				 SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_ECDH_KEY);
				 goto f_err;
				 }
			if (((clnt_pub_pkey=X509_get_pubkey(s->session->peer))
			    == NULL) || 
			    (clnt_pub_pkey->type != EVP_PKEY_EC))
				{
				/* XXX: For now, we do not support client
				 * authentication using ECDH certificates
				 * so this branch (n == 0L) of the code is
				 * never executed. When that support is
				 * added, we ought to ensure the key 
				 * received in the certificate is 
				 * authorized for key agreement.
				 * ECDH_compute_key implicitly checks that
				 * the two ECDH shares are for the same
				 * group.
				 */
			   	al=SSL_AD_HANDSHAKE_FAILURE;
			   	SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    SSL_R_UNABLE_TO_DECODE_ECDH_CERTS);
			   	goto f_err;
			   	}

			if (EC_POINT_copy(clnt_ecpoint,
			    EC_KEY_get0_public_key(clnt_pub_pkey->pkey.ec)) == 0)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_EC_LIB);
				goto err;
				}
			ret = 2; /* Skip certificate verify processing */
			}
		else
			{
			/* Get client's public key from encoded point
			 * in the ClientKeyExchange message.
			 */
			if ((bn_ctx = BN_CTX_new()) == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    ERR_R_MALLOC_FAILURE);
				goto err;
				}

			/* Get encoded point length */
			i = *p; 
			p += 1;
			if (EC_POINT_oct2point(group, 
			    clnt_ecpoint, p, i, bn_ctx) == 0)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    ERR_R_EC_LIB);
				goto err;
				}
			/* p is pointing to somewhere in the buffer
			 * currently, so set it to the start 
			 */ 
			p=(unsigned char *)s->init_buf->data;
			}

		/* Compute the shared pre-master secret */
		field_size = EC_GROUP_get_degree(group);
		if (field_size <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE, 
			       ERR_R_ECDH_LIB);
			goto err;
			}
		i = ECDH_compute_key(p, (field_size+7)/8, clnt_ecpoint, srvr_ecdh, NULL);
		if (i <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_ECDH_LIB);
			goto err;
			}

		EVP_PKEY_free(clnt_pub_pkey);
		EC_POINT_free(clnt_ecpoint);
		EC_KEY_free(srvr_ecdh);
		BN_CTX_free(bn_ctx);
		EC_KEY_free(s->s3->tmp.ecdh);
		s->s3->tmp.ecdh = NULL; 

		/* Compute the master secret */
		s->session->master_key_length = s->method->ssl3_enc-> \
		    generate_master_secret(s, s->session->master_key, p, i);
		
		OPENSSL_cleanse(p, i);
		return (ret);
		}
	else
#endif
#ifndef OPENSSL_NO_PSK
		if (alg_k & SSL_kPSK)
			{
			unsigned char *t = NULL;
			unsigned char psk_or_pre_ms[PSK_MAX_PSK_LEN*2+4];
			unsigned int pre_ms_len = 0, psk_len = 0;
			int psk_err = 1;
			char tmp_id[PSK_MAX_IDENTITY_LEN+1];

			al=SSL_AD_HANDSHAKE_FAILURE;

			n2s(p,i);
			if (n != i+2)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					SSL_R_LENGTH_MISMATCH);
				goto psk_err;
				}
			if (i > PSK_MAX_IDENTITY_LEN)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					SSL_R_DATA_LENGTH_TOO_LONG);
				goto psk_err;
				}
			if (s->psk_server_callback == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				       SSL_R_PSK_NO_SERVER_CB);
				goto psk_err;
				}

			/* Create guaranteed NULL-terminated identity
			 * string for the callback */
			memcpy(tmp_id, p, i);
			memset(tmp_id+i, 0, PSK_MAX_IDENTITY_LEN+1-i);
			psk_len = s->psk_server_callback(s, tmp_id,
				psk_or_pre_ms, sizeof(psk_or_pre_ms));
			OPENSSL_cleanse(tmp_id, PSK_MAX_IDENTITY_LEN+1);

			if (psk_len > PSK_MAX_PSK_LEN)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_INTERNAL_ERROR);
				goto psk_err;
				}
			else if (psk_len == 0)
				{
				/* PSK related to the given identity not found */
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				       SSL_R_PSK_IDENTITY_NOT_FOUND);
				al=SSL_AD_UNKNOWN_PSK_IDENTITY;
				goto psk_err;
				}

			/* create PSK pre_master_secret */
			pre_ms_len=2+psk_len+2+psk_len;
			t = psk_or_pre_ms;
			memmove(psk_or_pre_ms+psk_len+4, psk_or_pre_ms, psk_len);
			s2n(psk_len, t);
			memset(t, 0, psk_len);
			t+=psk_len;
			s2n(psk_len, t);

			if (s->session->psk_identity != NULL)
				OPENSSL_free(s->session->psk_identity);
			s->session->psk_identity = BUF_strdup((char *)p);
			if (s->session->psk_identity == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_MALLOC_FAILURE);
				goto psk_err;
				}

			if (s->session->psk_identity_hint != NULL)
				OPENSSL_free(s->session->psk_identity_hint);
			s->session->psk_identity_hint = BUF_strdup(s->ctx->psk_identity_hint);
			if (s->ctx->psk_identity_hint != NULL &&
				s->session->psk_identity_hint == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_MALLOC_FAILURE);
				goto psk_err;
				}

			s->session->master_key_length=
				s->method->ssl3_enc->generate_master_secret(s,
					s->session->master_key, psk_or_pre_ms, pre_ms_len);
			psk_err = 0;
		psk_err:
			OPENSSL_cleanse(psk_or_pre_ms, sizeof(psk_or_pre_ms));
			if (psk_err != 0)
				goto f_err;
			}
		else
#endif
		if (alg_k & SSL_kGOST) 
			{
			int ret = 0;
			EVP_PKEY_CTX *pkey_ctx;
			EVP_PKEY *client_pub_pkey = NULL, *pk = NULL;
			unsigned char premaster_secret[32], *start;
			size_t outlen=32, inlen;
			unsigned long alg_a;

			/* Get our certificate private key*/
			alg_a = s->s3->tmp.new_cipher->algorithm_auth;
			if (alg_a & SSL_aGOST94)
				pk = s->cert->pkeys[SSL_PKEY_GOST94].privatekey;
			else if (alg_a & SSL_aGOST01)
				pk = s->cert->pkeys[SSL_PKEY_GOST01].privatekey;

			pkey_ctx = EVP_PKEY_CTX_new(pk,NULL);
			EVP_PKEY_decrypt_init(pkey_ctx);
			/* If client certificate is present and is of the same type, maybe
			 * use it for key exchange.  Don't mind errors from
			 * EVP_PKEY_derive_set_peer, because it is completely valid to use
			 * a client certificate for authorization only. */
			client_pub_pkey = X509_get_pubkey(s->session->peer);
			if (client_pub_pkey)
				{
				if (EVP_PKEY_derive_set_peer(pkey_ctx, client_pub_pkey) <= 0)
					ERR_clear_error();
				}
			/* Decrypt session key */
			if ((*p!=( V_ASN1_SEQUENCE| V_ASN1_CONSTRUCTED))) 
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			if (p[1] == 0x81)
				{
				start = p+3;
				inlen = p[2];
				}
			else if (p[1] < 0x80)
				{
				start = p+2;
				inlen = p[1];
				}
			else
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			if (EVP_PKEY_decrypt(pkey_ctx,premaster_secret,&outlen,start,inlen) <=0) 

				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			/* Generate master secret */
			s->session->master_key_length=
				s->method->ssl3_enc->generate_master_secret(s,
					s->session->master_key,premaster_secret,32);
			/* Check if pubkey from client certificate was used */
			if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_PEER_KEY, 2, NULL) > 0)
				ret = 2;
			else
				ret = 1;
		gerr:
			EVP_PKEY_free(client_pub_pkey);
			EVP_PKEY_CTX_free(pkey_ctx);
			if (ret)
				return ret;
			else
				goto err;
			}
		else
		{
		al=SSL_AD_HANDSHAKE_FAILURE;
		SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_UNKNOWN_CIPHER_TYPE);
		goto f_err;
		}

	return(1);
f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
#if !defined(OPENSSL_NO_DH) || !defined(OPENSSL_NO_RSA) || !defined(OPENSSL_NO_ECDH)
err:
#endif
#ifndef OPENSSL_NO_ECDH
	EVP_PKEY_free(clnt_pub_pkey);
	EC_POINT_free(clnt_ecpoint);
	if (srvr_ecdh != NULL) 
		EC_KEY_free(srvr_ecdh);
	BN_CTX_free(bn_ctx);
#endif
	return(-1);
	}