long ssl3_ctrl(SSL *s, int cmd, long larg, void *parg)
	{
	int ret=0;

#if !defined(OPENSSL_NO_DSA) || !defined(OPENSSL_NO_RSA)
	if (
#ifndef OPENSSL_NO_RSA
	    cmd == SSL_CTRL_SET_TMP_RSA ||
	    cmd == SSL_CTRL_SET_TMP_RSA_CB ||
#endif
#ifndef OPENSSL_NO_DSA
	    cmd == SSL_CTRL_SET_TMP_DH ||
	    cmd == SSL_CTRL_SET_TMP_DH_CB ||
#endif
		0)
		{
		if (!ssl_cert_inst(&s->cert))
		    	{
			SSLerr(SSL_F_SSL3_CTRL, ERR_R_MALLOC_FAILURE);
			return(0);
			}
		}
#endif

	switch (cmd)
		{
	case SSL_CTRL_GET_SESSION_REUSED:
		ret=s->hit;
		break;
	case SSL_CTRL_GET_CLIENT_CERT_REQUEST:
		break;
	case SSL_CTRL_GET_NUM_RENEGOTIATIONS:
		ret=s->s3->num_renegotiations;
		break;
	case SSL_CTRL_CLEAR_NUM_RENEGOTIATIONS:
		ret=s->s3->num_renegotiations;
		s->s3->num_renegotiations=0;
		break;
	case SSL_CTRL_GET_TOTAL_RENEGOTIATIONS:
		ret=s->s3->total_renegotiations;
		break;
	case SSL_CTRL_GET_FLAGS:
		ret=(int)(s->s3->flags);
		break;
#ifndef OPENSSL_NO_RSA
	case SSL_CTRL_NEED_TMP_RSA:
		if ((s->cert != NULL) && (s->cert->rsa_tmp == NULL) &&
		    ((s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey == NULL) ||
		     (EVP_PKEY_size(s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey) > (512/8))))
			ret = 1;
		break;
	case SSL_CTRL_SET_TMP_RSA:
		{
			RSA *rsa = (RSA *)parg;
			if (rsa == NULL)
				{
				SSLerr(SSL_F_SSL3_CTRL, ERR_R_PASSED_NULL_PARAMETER);
				return(ret);
				}
			if ((rsa = RSAPrivateKey_dup(rsa)) == NULL)
				{
				SSLerr(SSL_F_SSL3_CTRL, ERR_R_RSA_LIB);
				return(ret);
				}
			if (s->cert->rsa_tmp != NULL)
				RSA_free(s->cert->rsa_tmp);
			s->cert->rsa_tmp = rsa;
			ret = 1;
		}
		break;
	case SSL_CTRL_SET_TMP_RSA_CB:
		{
		SSLerr(SSL_F_SSL3_CTRL, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return(ret);
		}
		break;
#endif
#ifndef OPENSSL_NO_DH
	case SSL_CTRL_SET_TMP_DH:
		{
			DH *dh = (DH *)parg;
			if (dh == NULL)
				{
				SSLerr(SSL_F_SSL3_CTRL, ERR_R_PASSED_NULL_PARAMETER);
				return(ret);
				}
			if (!ssl_security(s, SSL_SECOP_TMP_DH,
						DH_security_bits(dh), 0, dh))
				{
				SSLerr(SSL_F_SSL3_CTRL, SSL_R_DH_KEY_TOO_SMALL);
				return(ret);
				}
			if ((dh = DHparams_dup(dh)) == NULL)
				{
				SSLerr(SSL_F_SSL3_CTRL, ERR_R_DH_LIB);
				return(ret);
				}
			if (!(s->options & SSL_OP_SINGLE_DH_USE))
				{
				if (!DH_generate_key(dh))
					{
					DH_free(dh);
					SSLerr(SSL_F_SSL3_CTRL, ERR_R_DH_LIB);
					return(ret);
					}
				}
			if (s->cert->dh_tmp != NULL)
				DH_free(s->cert->dh_tmp);
			s->cert->dh_tmp = dh;
			ret = 1;
		}
		break;
	case SSL_CTRL_SET_TMP_DH_CB:
		{
		SSLerr(SSL_F_SSL3_CTRL, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return(ret);
		}
		break;
	case SSL_CTRL_SET_DH_AUTO:
		s->cert->dh_tmp_auto = larg;
		return 1;
#endif
#ifndef OPENSSL_NO_ECDH
	case SSL_CTRL_SET_TMP_ECDH:
		{
		EC_KEY *ecdh = NULL;
 			
		if (parg == NULL)
			{
			SSLerr(SSL_F_SSL3_CTRL, ERR_R_PASSED_NULL_PARAMETER);
			return(ret);
			}
		if (!EC_KEY_up_ref((EC_KEY *)parg))
			{
			SSLerr(SSL_F_SSL3_CTRL,ERR_R_ECDH_LIB);
			return(ret);
			}
		ecdh = (EC_KEY *)parg;
		if (!(s->options & SSL_OP_SINGLE_ECDH_USE))
			{
			if (!EC_KEY_generate_key(ecdh))
				{
				EC_KEY_free(ecdh);
				SSLerr(SSL_F_SSL3_CTRL,ERR_R_ECDH_LIB);
				return(ret);
				}
			}
		if (s->cert->ecdh_tmp != NULL)
			EC_KEY_free(s->cert->ecdh_tmp);
		s->cert->ecdh_tmp = ecdh;
		ret = 1;
		}
		break;
	case SSL_CTRL_SET_TMP_ECDH_CB:
		{
		SSLerr(SSL_F_SSL3_CTRL, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return(ret);
		}
		break;
#endif /* !OPENSSL_NO_ECDH */
#ifndef OPENSSL_NO_TLSEXT
	case SSL_CTRL_SET_TLSEXT_HOSTNAME:
 		if (larg == TLSEXT_NAMETYPE_host_name)
			{
			if (s->tlsext_hostname != NULL) 
				OPENSSL_free(s->tlsext_hostname);
			s->tlsext_hostname = NULL;

			ret = 1;
			if (parg == NULL) 
				break;
			if (strlen((char *)parg) > TLSEXT_MAXLEN_host_name)
				{
				SSLerr(SSL_F_SSL3_CTRL, SSL_R_SSL3_EXT_INVALID_SERVERNAME);
				return 0;
				}
			if ((s->tlsext_hostname = BUF_strdup((char *)parg)) == NULL)
				{
				SSLerr(SSL_F_SSL3_CTRL, ERR_R_INTERNAL_ERROR);
				return 0;
				}
			}
		else
			{
			SSLerr(SSL_F_SSL3_CTRL, SSL_R_SSL3_EXT_INVALID_SERVERNAME_TYPE);
			return 0;
			}
 		break;
	case SSL_CTRL_SET_TLSEXT_DEBUG_ARG:
		s->tlsext_debug_arg=parg;
		ret = 1;
		break;

#ifdef TLSEXT_TYPE_opaque_prf_input
	case SSL_CTRL_SET_TLSEXT_OPAQUE_PRF_INPUT:
		if (larg > 12288) /* actual internal limit is 2^16 for the complete hello message
		                   * (including the cert chain and everything) */
			{
			SSLerr(SSL_F_SSL3_CTRL, SSL_R_OPAQUE_PRF_INPUT_TOO_LONG);
			break;
			}
		if (s->tlsext_opaque_prf_input != NULL)
			OPENSSL_free(s->tlsext_opaque_prf_input);
		if ((size_t)larg == 0)
			s->tlsext_opaque_prf_input = OPENSSL_malloc(1); /* dummy byte just to get non-NULL */
		else
			s->tlsext_opaque_prf_input = BUF_memdup(parg, (size_t)larg);
		if (s->tlsext_opaque_prf_input != NULL)
			{
			s->tlsext_opaque_prf_input_len = (size_t)larg;
			ret = 1;
			}
		else
			s->tlsext_opaque_prf_input_len = 0;
		break;
#endif

	case SSL_CTRL_SET_TLSEXT_STATUS_REQ_TYPE:
		s->tlsext_status_type=larg;
		ret = 1;
		break;

	case SSL_CTRL_GET_TLSEXT_STATUS_REQ_EXTS:
		*(STACK_OF(X509_EXTENSION) **)parg = s->tlsext_ocsp_exts;
		ret = 1;
		break;

	case SSL_CTRL_SET_TLSEXT_STATUS_REQ_EXTS:
		s->tlsext_ocsp_exts = parg;
		ret = 1;
		break;

	case SSL_CTRL_GET_TLSEXT_STATUS_REQ_IDS:
		*(STACK_OF(OCSP_RESPID) **)parg = s->tlsext_ocsp_ids;
		ret = 1;
		break;

	case SSL_CTRL_SET_TLSEXT_STATUS_REQ_IDS:
		s->tlsext_ocsp_ids = parg;
		ret = 1;
		break;

	case SSL_CTRL_GET_TLSEXT_STATUS_REQ_OCSP_RESP:
		*(unsigned char **)parg = s->tlsext_ocsp_resp;
		return s->tlsext_ocsp_resplen;
		
	case SSL_CTRL_SET_TLSEXT_STATUS_REQ_OCSP_RESP:
		if (s->tlsext_ocsp_resp)
			OPENSSL_free(s->tlsext_ocsp_resp);
		s->tlsext_ocsp_resp = parg;
		s->tlsext_ocsp_resplen = larg;
		ret = 1;
		break;

#ifndef OPENSSL_NO_HEARTBEATS
	case SSL_CTRL_TLS_EXT_SEND_HEARTBEAT:
		if (SSL_IS_DTLS(s))
			ret = dtls1_heartbeat(s);
		else
			ret = tls1_heartbeat(s);
		break;

	case SSL_CTRL_GET_TLS_EXT_HEARTBEAT_PENDING:
		ret = s->tlsext_hb_pending;
		break;

	case SSL_CTRL_SET_TLS_EXT_HEARTBEAT_NO_REQUESTS:
		if (larg)
			s->tlsext_heartbeat |= SSL_TLSEXT_HB_DONT_RECV_REQUESTS;
		else
			s->tlsext_heartbeat &= ~SSL_TLSEXT_HB_DONT_RECV_REQUESTS;
		ret = 1;
		break;
#endif

#endif /* !OPENSSL_NO_TLSEXT */

	case SSL_CTRL_CHAIN:
		if (larg)
			return ssl_cert_set1_chain(s, NULL,
						(STACK_OF (X509) *)parg);
		else
			return ssl_cert_set0_chain(s, NULL,
						(STACK_OF (X509) *)parg);

	case SSL_CTRL_CHAIN_CERT:
		if (larg)
			return ssl_cert_add1_chain_cert(s, NULL, (X509 *)parg);
		else
			return ssl_cert_add0_chain_cert(s, NULL, (X509 *)parg);

	case SSL_CTRL_GET_CHAIN_CERTS:
		*(STACK_OF(X509) **)parg = s->cert->key->chain;
		break;

	case SSL_CTRL_SELECT_CURRENT_CERT:
		return ssl_cert_select_current(s->cert, (X509 *)parg);

	case SSL_CTRL_SET_CURRENT_CERT:
		if (larg == SSL_CERT_SET_SERVER)
			{
			CERT_PKEY *cpk;
			const SSL_CIPHER *cipher;
			if (!s->server)
				return 0;
			cipher = s->s3->tmp.new_cipher;
			if (!cipher)
				return 0;
			/* No certificate for unauthenticated ciphersuites
			 * or using SRP authentication
			 */
			if (cipher->algorithm_auth & (SSL_aNULL|SSL_aSRP))
				return 2;
			cpk = ssl_get_server_send_pkey(s);
			if (!cpk)
				return 0;
			s->cert->key = cpk;
			return 1;
			}
		return ssl_cert_set_current(s->cert, larg);

#ifndef OPENSSL_NO_EC
	case SSL_CTRL_GET_CURVES:
		{
		unsigned char *clist;
		size_t clistlen;
		if (!s->session)
			return 0;
		clist = s->session->tlsext_ellipticcurvelist;
		clistlen = s->session->tlsext_ellipticcurvelist_length / 2;
		if (parg)
			{
			size_t i;
			int *cptr = parg;
			unsigned int cid, nid;
			for (i = 0; i < clistlen; i++)
				{
				n2s(clist, cid);
				nid = tls1_ec_curve_id2nid(cid);
				if (nid != 0)
					cptr[i] = nid;
				else
					cptr[i] = TLSEXT_nid_unknown | cid;
				}
			}
		return (int)clistlen;
		}

	case SSL_CTRL_SET_CURVES:
		return tls1_set_curves(&s->tlsext_ellipticcurvelist,
					&s->tlsext_ellipticcurvelist_length,
								parg, larg);

	case SSL_CTRL_SET_CURVES_LIST:
		return tls1_set_curves_list(&s->tlsext_ellipticcurvelist,
					&s->tlsext_ellipticcurvelist_length,
								parg);

	case SSL_CTRL_GET_SHARED_CURVE:
		return tls1_shared_curve(s, larg);

	case SSL_CTRL_SET_ECDH_AUTO:
		s->cert->ecdh_tmp_auto = larg;
		return 1;
#endif
	case SSL_CTRL_SET_SIGALGS:
		return tls1_set_sigalgs(s->cert, parg, larg, 0);

	case SSL_CTRL_SET_SIGALGS_LIST:
		return tls1_set_sigalgs_list(s->cert, parg, 0);

	case SSL_CTRL_SET_CLIENT_SIGALGS:
		return tls1_set_sigalgs(s->cert, parg, larg, 1);

	case SSL_CTRL_SET_CLIENT_SIGALGS_LIST:
		return tls1_set_sigalgs_list(s->cert, parg, 1);

	case SSL_CTRL_GET_CLIENT_CERT_TYPES:
		{
		const unsigned char **pctype = parg;
		if (s->server || !s->s3->tmp.cert_req)
			return 0;
		if (s->cert->ctypes)
			{
			if (pctype)
				*pctype = s->cert->ctypes;
			return (int)s->cert->ctype_num;
			}
		if (pctype)
			*pctype = (unsigned char *)s->s3->tmp.ctype;
		return s->s3->tmp.ctype_num;
		}

	case SSL_CTRL_SET_CLIENT_CERT_TYPES:
		if (!s->server)
			return 0;
		return ssl3_set_req_cert_type(s->cert, parg, larg);

	case SSL_CTRL_BUILD_CERT_CHAIN:
		return ssl_build_cert_chain(s, NULL, larg);

	case SSL_CTRL_SET_VERIFY_CERT_STORE:
		return ssl_cert_set_cert_store(s->cert, parg, 0, larg);

	case SSL_CTRL_SET_CHAIN_CERT_STORE:
		return ssl_cert_set_cert_store(s->cert, parg, 1, larg);

	case SSL_CTRL_GET_PEER_SIGNATURE_NID:
		if (SSL_USE_SIGALGS(s))
			{
			if (s->session && s->session->sess_cert)
				{
				const EVP_MD *sig;
				sig = s->session->sess_cert->peer_key->digest;
				if (sig)
					{
					*(int *)parg = EVP_MD_type(sig);
					return 1;
					}
				}
			return 0;
			}
		/* Might want to do something here for other versions */
		else
			return 0;

	case SSL_CTRL_GET_SERVER_TMP_KEY:
		if (s->server || !s->session || !s->session->sess_cert)
			return 0;
		else
			{
			SESS_CERT *sc;
			EVP_PKEY *ptmp;
			int rv = 0;
			sc = s->session->sess_cert;
#if !defined(OPENSSL_NO_RSA) && !defined(OPENSSL_NO_DH) && !defined(OPENSSL_NO_EC)
			if (!sc->peer_rsa_tmp && !sc->peer_dh_tmp
							&& !sc->peer_ecdh_tmp)
				return 0;
#endif
			ptmp = EVP_PKEY_new();
			if (!ptmp)
				return 0;
			if (0);
#ifndef OPENSSL_NO_RSA
			else if (sc->peer_rsa_tmp)
				rv = EVP_PKEY_set1_RSA(ptmp, sc->peer_rsa_tmp);
#endif
#ifndef OPENSSL_NO_DH
			else if (sc->peer_dh_tmp)
				rv = EVP_PKEY_set1_DH(ptmp, sc->peer_dh_tmp);
#endif
#ifndef OPENSSL_NO_ECDH
			else if (sc->peer_ecdh_tmp)
				rv = EVP_PKEY_set1_EC_KEY(ptmp, sc->peer_ecdh_tmp);
#endif
			if (rv)
				{
				*(EVP_PKEY **)parg = ptmp;
				return 1;
				}
			EVP_PKEY_free(ptmp);
			return 0;
			}
#ifndef OPENSSL_NO_EC
	case SSL_CTRL_GET_EC_POINT_FORMATS:
		{
		SSL_SESSION *sess = s->session;
		const unsigned char **pformat = parg;
		if (!sess || !sess->tlsext_ecpointformatlist)
			return 0;
		*pformat = sess->tlsext_ecpointformatlist;
		return (int)sess->tlsext_ecpointformatlist_length;
		}
#endif
	default:
		break;
		}
	return(ret);
	}