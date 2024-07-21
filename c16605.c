int ssl3_send_client_certificate(SSL *s)
	{
	X509 *x509=NULL;
	EVP_PKEY *pkey=NULL;
	int i;
	unsigned long l;

	if (s->state ==	SSL3_ST_CW_CERT_A)
		{
		if ((s->cert == NULL) ||
			(s->cert->key->x509 == NULL) ||
			(s->cert->key->privatekey == NULL))
			s->state=SSL3_ST_CW_CERT_B;
		else
			s->state=SSL3_ST_CW_CERT_C;
		}

	/* We need to get a client cert */
	if (s->state == SSL3_ST_CW_CERT_B)
		{
		/* If we get an error, we need to
		 * ssl->rwstate=SSL_X509_LOOKUP; return(-1);
		 * We then get retied later */
		i=0;
		i = ssl_do_client_cert_cb(s, &x509, &pkey);
		if (i < 0)
			{
			s->rwstate=SSL_X509_LOOKUP;
			return(-1);
			}
		s->rwstate=SSL_NOTHING;
		if ((i == 1) && (pkey != NULL) && (x509 != NULL))
			{
			s->state=SSL3_ST_CW_CERT_B;
			if (	!SSL_use_certificate(s,x509) ||
				!SSL_use_PrivateKey(s,pkey))
				i=0;
			}
		else if (i == 1)
			{
			i=0;
			SSLerr(SSL_F_SSL3_SEND_CLIENT_CERTIFICATE,SSL_R_BAD_DATA_RETURNED_BY_CALLBACK);
			}

		if (x509 != NULL) X509_free(x509);
		if (pkey != NULL) EVP_PKEY_free(pkey);
		if (i == 0)
			{
			if (s->version == SSL3_VERSION)
				{
				s->s3->tmp.cert_req=0;
				ssl3_send_alert(s,SSL3_AL_WARNING,SSL_AD_NO_CERTIFICATE);
				return(1);
				}
			else
				{
				s->s3->tmp.cert_req=2;
				}
			}

		/* Ok, we have a cert */
		s->state=SSL3_ST_CW_CERT_C;
		}

	if (s->state == SSL3_ST_CW_CERT_C)
		{
		s->state=SSL3_ST_CW_CERT_D;
		l=ssl3_output_cert_chain(s,
			(s->s3->tmp.cert_req == 2)?NULL:s->cert->key->x509);
		s->init_num=(int)l;
		s->init_off=0;
		}
	/* SSL3_ST_CW_CERT_D */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}