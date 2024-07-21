SSL_SESSION *d2i_SSL_SESSION(SSL_SESSION **a, const unsigned char **pp,
			     long length)
	{
	int ssl_version=0,i;
	long id;
	ASN1_INTEGER ai,*aip;
	ASN1_OCTET_STRING os,*osp;
	M_ASN1_D2I_vars(a,SSL_SESSION *,SSL_SESSION_new);

	aip= &ai;
	osp= &os;

	M_ASN1_D2I_Init();
	M_ASN1_D2I_start_sequence();

	ai.data=NULL; ai.length=0;
	M_ASN1_D2I_get_x(ASN1_INTEGER,aip,d2i_ASN1_INTEGER);
	if (ai.data != NULL) { OPENSSL_free(ai.data); ai.data=NULL; ai.length=0; }

	/* we don't care about the version right now :-) */
	M_ASN1_D2I_get_x(ASN1_INTEGER,aip,d2i_ASN1_INTEGER);
	ssl_version=(int)ASN1_INTEGER_get(aip);
	ret->ssl_version=ssl_version;
	if (ai.data != NULL) { OPENSSL_free(ai.data); ai.data=NULL; ai.length=0; }

	os.data=NULL; os.length=0;
	M_ASN1_D2I_get_x(ASN1_OCTET_STRING,osp,d2i_ASN1_OCTET_STRING);
	if (ssl_version == SSL2_VERSION)
		{
		if (os.length != 3)
			{
			c.error=SSL_R_CIPHER_CODE_WRONG_LENGTH;
			goto err;
			}
		id=0x02000000L|
			((unsigned long)os.data[0]<<16L)|
			((unsigned long)os.data[1]<< 8L)|
			 (unsigned long)os.data[2];
		}
	else if ((ssl_version>>8) >= SSL3_VERSION_MAJOR)
		{
		if (os.length != 2)
			{
			c.error=SSL_R_CIPHER_CODE_WRONG_LENGTH;
			goto err;
			}
		id=0x03000000L|
			((unsigned long)os.data[0]<<8L)|
			 (unsigned long)os.data[1];
		}
	else
		{
		c.error=SSL_R_UNKNOWN_SSL_VERSION;
		goto err;
		}
	
	ret->cipher=NULL;
	ret->cipher_id=id;

	M_ASN1_D2I_get_x(ASN1_OCTET_STRING,osp,d2i_ASN1_OCTET_STRING);
	if ((ssl_version>>8) >= SSL3_VERSION_MAJOR)
		i=SSL3_MAX_SSL_SESSION_ID_LENGTH;
	else /* if (ssl_version>>8 == SSL2_VERSION_MAJOR) */
		i=SSL2_MAX_SSL_SESSION_ID_LENGTH;

	if (os.length > i)
		os.length = i;
	if (os.length > (int)sizeof(ret->session_id)) /* can't happen */
		os.length = sizeof(ret->session_id);

	ret->session_id_length=os.length;
	OPENSSL_assert(os.length <= (int)sizeof(ret->session_id));
	memcpy(ret->session_id,os.data,os.length);

	M_ASN1_D2I_get_x(ASN1_OCTET_STRING,osp,d2i_ASN1_OCTET_STRING);
	if (os.length > SSL_MAX_MASTER_KEY_LENGTH)
		ret->master_key_length=SSL_MAX_MASTER_KEY_LENGTH;
	else
		ret->master_key_length=os.length;
	memcpy(ret->master_key,os.data,ret->master_key_length);

	os.length=0;

#ifndef OPENSSL_NO_KRB5
	os.length=0;
	M_ASN1_D2I_get_opt(osp,d2i_ASN1_OCTET_STRING,V_ASN1_OCTET_STRING);
	if (os.data)
		{
        	if (os.length > SSL_MAX_KRB5_PRINCIPAL_LENGTH)
            		ret->krb5_client_princ_len=0;
		else
			ret->krb5_client_princ_len=os.length;
		memcpy(ret->krb5_client_princ,os.data,ret->krb5_client_princ_len);
		OPENSSL_free(os.data);
		os.data = NULL;
		os.length = 0;
		}
	else
		ret->krb5_client_princ_len=0;
#endif /* OPENSSL_NO_KRB5 */

	M_ASN1_D2I_get_IMP_opt(osp,d2i_ASN1_OCTET_STRING,0,V_ASN1_OCTET_STRING);
	if (os.length > SSL_MAX_KEY_ARG_LENGTH)
		ret->key_arg_length=SSL_MAX_KEY_ARG_LENGTH;
	else
		ret->key_arg_length=os.length;
	memcpy(ret->key_arg,os.data,ret->key_arg_length);
	if (os.data != NULL) OPENSSL_free(os.data);

	ai.length=0;
	M_ASN1_D2I_get_EXP_opt(aip,d2i_ASN1_INTEGER,1);
	if (ai.data != NULL)
		{
		ret->time=ASN1_INTEGER_get(aip);
		OPENSSL_free(ai.data); ai.data=NULL; ai.length=0;
		}
	else
		ret->time=(unsigned long)time(NULL);

	ai.length=0;
	M_ASN1_D2I_get_EXP_opt(aip,d2i_ASN1_INTEGER,2);
	if (ai.data != NULL)
		{
		ret->timeout=ASN1_INTEGER_get(aip);
		OPENSSL_free(ai.data); ai.data=NULL; ai.length=0;
		}
	else
		ret->timeout=3;

	if (ret->peer != NULL)
		{
		X509_free(ret->peer);
		ret->peer=NULL;
		}
	M_ASN1_D2I_get_EXP_opt(ret->peer,d2i_X509,3);

	os.length=0;
	os.data=NULL;
	M_ASN1_D2I_get_EXP_opt(osp,d2i_ASN1_OCTET_STRING,4);

	if(os.data != NULL)
	    {
	    if (os.length > SSL_MAX_SID_CTX_LENGTH)
		{
		c.error=SSL_R_BAD_LENGTH;
		goto err;
		}
	    else
		{
		ret->sid_ctx_length=os.length;
		memcpy(ret->sid_ctx,os.data,os.length);
		}
	    OPENSSL_free(os.data); os.data=NULL; os.length=0;
	    }
	else
	    ret->sid_ctx_length=0;

	ai.length=0;
	M_ASN1_D2I_get_EXP_opt(aip,d2i_ASN1_INTEGER,5);
	if (ai.data != NULL)
		{
		ret->verify_result=ASN1_INTEGER_get(aip);
		OPENSSL_free(ai.data); ai.data=NULL; ai.length=0;
		}
	else
		ret->verify_result=X509_V_OK;

#ifndef OPENSSL_NO_TLSEXT
	os.length=0;
	os.data=NULL;
	M_ASN1_D2I_get_EXP_opt(osp,d2i_ASN1_OCTET_STRING,6);
	if (os.data)
		{
		ret->tlsext_hostname = BUF_strndup((char *)os.data, os.length);
		OPENSSL_free(os.data);
		os.data = NULL;
		os.length = 0;
		}
	else
		ret->tlsext_hostname=NULL;
#endif /* OPENSSL_NO_TLSEXT */

#ifndef OPENSSL_NO_PSK
	os.length=0;
	os.data=NULL;
	M_ASN1_D2I_get_EXP_opt(osp,d2i_ASN1_OCTET_STRING,7);
	if (os.data)
		{
		ret->psk_identity_hint = BUF_strndup((char *)os.data, os.length);
		OPENSSL_free(os.data);
		os.data = NULL;
		os.length = 0;
		}
	else
		ret->psk_identity_hint=NULL;
#endif /* OPENSSL_NO_PSK */

#ifndef OPENSSL_NO_TLSEXT
	ai.length=0;
	M_ASN1_D2I_get_EXP_opt(aip,d2i_ASN1_INTEGER,9);
	if (ai.data != NULL)
		{
		ret->tlsext_tick_lifetime_hint=ASN1_INTEGER_get(aip);
		OPENSSL_free(ai.data); ai.data=NULL; ai.length=0;
		}
	else if (ret->tlsext_ticklen && ret->session_id_length)
		ret->tlsext_tick_lifetime_hint = -1;
	else
		ret->tlsext_tick_lifetime_hint=0;
	os.length=0;
	os.data=NULL;
	M_ASN1_D2I_get_EXP_opt(osp,d2i_ASN1_OCTET_STRING,10);
	if (os.data)
		{
		ret->tlsext_tick = os.data;
		ret->tlsext_ticklen = os.length;
		os.data = NULL;
		os.length = 0;
		}
	else
		ret->tlsext_tick=NULL;
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_COMP
	os.length=0;
	os.data=NULL;
	M_ASN1_D2I_get_EXP_opt(osp,d2i_ASN1_OCTET_STRING,11);
	if (os.data)
		{
		ret->compress_meth = os.data[0];
		OPENSSL_free(os.data);
		os.data = NULL;
		}
#endif

	M_ASN1_D2I_Finish(a,SSL_SESSION_free,SSL_F_D2I_SSL_SESSION);
	}