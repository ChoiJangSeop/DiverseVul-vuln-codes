int i2d_SSL_SESSION(SSL_SESSION *in, unsigned char **pp)
	{
#define LSIZE2 (sizeof(long)*2)
	int v1=0,v2=0,v3=0,v4=0,v5=0,v7=0,v8=0;
	unsigned char buf[4],ibuf1[LSIZE2],ibuf2[LSIZE2];
	unsigned char ibuf3[LSIZE2],ibuf4[LSIZE2],ibuf5[LSIZE2];
#ifndef OPENSSL_NO_TLSEXT
	int v6=0,v9=0,v10=0;
	unsigned char ibuf6[LSIZE2];
#endif
#ifndef OPENSSL_NO_COMP
	unsigned char cbuf;
	int v11=0;
#endif
	long l;
	SSL_SESSION_ASN1 a;
	M_ASN1_I2D_vars(in);

	if ((in == NULL) || ((in->cipher == NULL) && (in->cipher_id == 0)))
		return(0);

	/* Note that I cheat in the following 2 assignments.  I know
	 * that if the ASN1_INTEGER passed to ASN1_INTEGER_set
	 * is > sizeof(long)+1, the buffer will not be re-OPENSSL_malloc()ed.
	 * This is a bit evil but makes things simple, no dynamic allocation
	 * to clean up :-) */
	a.version.length=LSIZE2;
	a.version.type=V_ASN1_INTEGER;
	a.version.data=ibuf1;
	ASN1_INTEGER_set(&(a.version),SSL_SESSION_ASN1_VERSION);

	a.ssl_version.length=LSIZE2;
	a.ssl_version.type=V_ASN1_INTEGER;
	a.ssl_version.data=ibuf2;
	ASN1_INTEGER_set(&(a.ssl_version),in->ssl_version);

	a.cipher.type=V_ASN1_OCTET_STRING;
	a.cipher.data=buf;

	if (in->cipher == NULL)
		l=in->cipher_id;
	else
		l=in->cipher->id;
	if (in->ssl_version == SSL2_VERSION)
		{
		a.cipher.length=3;
		buf[0]=((unsigned char)(l>>16L))&0xff;
		buf[1]=((unsigned char)(l>> 8L))&0xff;
		buf[2]=((unsigned char)(l     ))&0xff;
		}
	else
		{
		a.cipher.length=2;
		buf[0]=((unsigned char)(l>>8L))&0xff;
		buf[1]=((unsigned char)(l    ))&0xff;
		}

#ifndef OPENSSL_NO_COMP
	if (in->compress_meth)
		{
		cbuf = (unsigned char)in->compress_meth;
		a.comp_id.length = 1;
		a.comp_id.type = V_ASN1_OCTET_STRING;
		a.comp_id.data = &cbuf;
		}
#endif

	a.master_key.length=in->master_key_length;
	a.master_key.type=V_ASN1_OCTET_STRING;
	a.master_key.data=in->master_key;

	a.session_id.length=in->session_id_length;
	a.session_id.type=V_ASN1_OCTET_STRING;
	a.session_id.data=in->session_id;

	a.session_id_context.length=in->sid_ctx_length;
	a.session_id_context.type=V_ASN1_OCTET_STRING;
	a.session_id_context.data=in->sid_ctx;

	a.key_arg.length=in->key_arg_length;
	a.key_arg.type=V_ASN1_OCTET_STRING;
	a.key_arg.data=in->key_arg;

#ifndef OPENSSL_NO_KRB5
	if (in->krb5_client_princ_len)
		{
		a.krb5_princ.length=in->krb5_client_princ_len;
		a.krb5_princ.type=V_ASN1_OCTET_STRING;
		a.krb5_princ.data=in->krb5_client_princ;
		}
#endif /* OPENSSL_NO_KRB5 */

	if (in->time != 0L)
		{
		a.time.length=LSIZE2;
		a.time.type=V_ASN1_INTEGER;
		a.time.data=ibuf3;
		ASN1_INTEGER_set(&(a.time),in->time);
		}

	if (in->timeout != 0L)
		{
		a.timeout.length=LSIZE2;
		a.timeout.type=V_ASN1_INTEGER;
		a.timeout.data=ibuf4;
		ASN1_INTEGER_set(&(a.timeout),in->timeout);
		}

	if (in->verify_result != X509_V_OK)
		{
		a.verify_result.length=LSIZE2;
		a.verify_result.type=V_ASN1_INTEGER;
		a.verify_result.data=ibuf5;
		ASN1_INTEGER_set(&a.verify_result,in->verify_result);
		}

#ifndef OPENSSL_NO_TLSEXT
	if (in->tlsext_hostname)
                {
                a.tlsext_hostname.length=strlen(in->tlsext_hostname);
                a.tlsext_hostname.type=V_ASN1_OCTET_STRING;
                a.tlsext_hostname.data=(unsigned char *)in->tlsext_hostname;
                }
	if (in->tlsext_tick)
                {
                a.tlsext_tick.length= in->tlsext_ticklen;
                a.tlsext_tick.type=V_ASN1_OCTET_STRING;
                a.tlsext_tick.data=(unsigned char *)in->tlsext_tick;
                }
	if (in->tlsext_tick_lifetime_hint > 0)
		{
		a.tlsext_tick_lifetime.length=LSIZE2;
		a.tlsext_tick_lifetime.type=V_ASN1_INTEGER;
		a.tlsext_tick_lifetime.data=ibuf6;
		ASN1_INTEGER_set(&a.tlsext_tick_lifetime,in->tlsext_tick_lifetime_hint);
		}
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_PSK
	if (in->psk_identity_hint)
		{
		a.psk_identity_hint.length=strlen(in->psk_identity_hint);
		a.psk_identity_hint.type=V_ASN1_OCTET_STRING;
		a.psk_identity_hint.data=(unsigned char *)(in->psk_identity_hint);
		}
	if (in->psk_identity)
		{
		a.psk_identity.length=strlen(in->psk_identity);
		a.psk_identity.type=V_ASN1_OCTET_STRING;
		a.psk_identity.data=(unsigned char *)(in->psk_identity);
		}
#endif /* OPENSSL_NO_PSK */

	M_ASN1_I2D_len(&(a.version),		i2d_ASN1_INTEGER);
	M_ASN1_I2D_len(&(a.ssl_version),	i2d_ASN1_INTEGER);
	M_ASN1_I2D_len(&(a.cipher),		i2d_ASN1_OCTET_STRING);
	M_ASN1_I2D_len(&(a.session_id),		i2d_ASN1_OCTET_STRING);
	M_ASN1_I2D_len(&(a.master_key),		i2d_ASN1_OCTET_STRING);
#ifndef OPENSSL_NO_KRB5
	if (in->krb5_client_princ_len)
        	M_ASN1_I2D_len(&(a.krb5_princ),	i2d_ASN1_OCTET_STRING);
#endif /* OPENSSL_NO_KRB5 */
	if (in->key_arg_length > 0)
		M_ASN1_I2D_len_IMP_opt(&(a.key_arg),i2d_ASN1_OCTET_STRING);
	if (in->time != 0L)
		M_ASN1_I2D_len_EXP_opt(&(a.time),i2d_ASN1_INTEGER,1,v1);
	if (in->timeout != 0L)
		M_ASN1_I2D_len_EXP_opt(&(a.timeout),i2d_ASN1_INTEGER,2,v2);
	if (in->peer != NULL)
		M_ASN1_I2D_len_EXP_opt(in->peer,i2d_X509,3,v3);
	M_ASN1_I2D_len_EXP_opt(&a.session_id_context,i2d_ASN1_OCTET_STRING,4,v4);
	if (in->verify_result != X509_V_OK)
		M_ASN1_I2D_len_EXP_opt(&(a.verify_result),i2d_ASN1_INTEGER,5,v5);

#ifndef OPENSSL_NO_TLSEXT
	if (in->tlsext_tick_lifetime_hint > 0)
      	 	M_ASN1_I2D_len_EXP_opt(&a.tlsext_tick_lifetime, i2d_ASN1_INTEGER,9,v9);
	if (in->tlsext_tick)
        	M_ASN1_I2D_len_EXP_opt(&(a.tlsext_tick), i2d_ASN1_OCTET_STRING,10,v10);
	if (in->tlsext_hostname)
        	M_ASN1_I2D_len_EXP_opt(&(a.tlsext_hostname), i2d_ASN1_OCTET_STRING,6,v6);
#ifndef OPENSSL_NO_COMP
	if (in->compress_meth)
        	M_ASN1_I2D_len_EXP_opt(&(a.comp_id), i2d_ASN1_OCTET_STRING,11,v11);
#endif
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_PSK
	if (in->psk_identity_hint)
        	M_ASN1_I2D_len_EXP_opt(&(a.psk_identity_hint), i2d_ASN1_OCTET_STRING,7,v7);
	if (in->psk_identity)
        	M_ASN1_I2D_len_EXP_opt(&(a.psk_identity), i2d_ASN1_OCTET_STRING,8,v8);
#endif /* OPENSSL_NO_PSK */

	M_ASN1_I2D_seq_total();

	M_ASN1_I2D_put(&(a.version),		i2d_ASN1_INTEGER);
	M_ASN1_I2D_put(&(a.ssl_version),	i2d_ASN1_INTEGER);
	M_ASN1_I2D_put(&(a.cipher),		i2d_ASN1_OCTET_STRING);
	M_ASN1_I2D_put(&(a.session_id),		i2d_ASN1_OCTET_STRING);
	M_ASN1_I2D_put(&(a.master_key),		i2d_ASN1_OCTET_STRING);
#ifndef OPENSSL_NO_KRB5
	if (in->krb5_client_princ_len)
        	M_ASN1_I2D_put(&(a.krb5_princ),	i2d_ASN1_OCTET_STRING);
#endif /* OPENSSL_NO_KRB5 */
	if (in->key_arg_length > 0)
		M_ASN1_I2D_put_IMP_opt(&(a.key_arg),i2d_ASN1_OCTET_STRING,0);
	if (in->time != 0L)
		M_ASN1_I2D_put_EXP_opt(&(a.time),i2d_ASN1_INTEGER,1,v1);
	if (in->timeout != 0L)
		M_ASN1_I2D_put_EXP_opt(&(a.timeout),i2d_ASN1_INTEGER,2,v2);
	if (in->peer != NULL)
		M_ASN1_I2D_put_EXP_opt(in->peer,i2d_X509,3,v3);
	M_ASN1_I2D_put_EXP_opt(&a.session_id_context,i2d_ASN1_OCTET_STRING,4,
			       v4);
	if (in->verify_result != X509_V_OK)
		M_ASN1_I2D_put_EXP_opt(&a.verify_result,i2d_ASN1_INTEGER,5,v5);
#ifndef OPENSSL_NO_TLSEXT
	if (in->tlsext_hostname)
        	M_ASN1_I2D_put_EXP_opt(&(a.tlsext_hostname), i2d_ASN1_OCTET_STRING,6,v6);
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_PSK
	if (in->psk_identity_hint)
		M_ASN1_I2D_put_EXP_opt(&(a.psk_identity_hint), i2d_ASN1_OCTET_STRING,7,v7);
	if (in->psk_identity)
		M_ASN1_I2D_put_EXP_opt(&(a.psk_identity), i2d_ASN1_OCTET_STRING,8,v8);
#endif /* OPENSSL_NO_PSK */
#ifndef OPENSSL_NO_TLSEXT
	if (in->tlsext_tick_lifetime_hint > 0)
      	 	M_ASN1_I2D_put_EXP_opt(&a.tlsext_tick_lifetime, i2d_ASN1_INTEGER,9,v9);
	if (in->tlsext_tick)
        	M_ASN1_I2D_put_EXP_opt(&(a.tlsext_tick), i2d_ASN1_OCTET_STRING,10,v10);
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_COMP
	if (in->compress_meth)
        	M_ASN1_I2D_put_EXP_opt(&(a.comp_id), i2d_ASN1_OCTET_STRING,11,v11);
#endif
	M_ASN1_I2D_finish();
	}