static void ssl_cipher_get_disabled(unsigned long *mkey, unsigned long *auth, unsigned long *enc, unsigned long *mac, unsigned long *ssl)
	{
	*mkey = 0;
	*auth = 0;
	*enc = 0;
	*mac = 0;
	*ssl = 0;

#ifdef OPENSSL_NO_RSA
	*mkey |= SSL_kRSA;
	*auth |= SSL_aRSA;
#endif
#ifdef OPENSSL_NO_DSA
	*auth |= SSL_aDSS;
#endif
	*mkey |= SSL_kDHr|SSL_kDHd; /* no such ciphersuites supported! */
	*auth |= SSL_aDH;
#ifdef OPENSSL_NO_DH
	*mkey |= SSL_kDHr|SSL_kDHd|SSL_kEDH;
	*auth |= SSL_aDH;
#endif
#ifdef OPENSSL_NO_KRB5
	*mkey |= SSL_kKRB5;
	*auth |= SSL_aKRB5;
#endif
#ifdef OPENSSL_NO_ECDSA
	*auth |= SSL_aECDSA;
#endif
#ifdef OPENSSL_NO_ECDH
	*mkey |= SSL_kECDHe|SSL_kECDHr;
	*auth |= SSL_aECDH;
#endif
#ifdef OPENSSL_NO_PSK
	*mkey |= SSL_kPSK;
	*auth |= SSL_aPSK;
#endif
	/* Check for presence of GOST 34.10 algorithms, and if they
	 * do not present, disable  appropriate auth and key exchange */
	if (!get_optional_pkey_id("gost94")) {
		*auth |= SSL_aGOST94;
	}
	if (!get_optional_pkey_id("gost2001")) {
		*auth |= SSL_aGOST01;
	}
	/* Disable GOST key exchange if no GOST signature algs are available * */
	if ((*auth & (SSL_aGOST94|SSL_aGOST01)) == (SSL_aGOST94|SSL_aGOST01)) {
		*mkey |= SSL_kGOST;
	}	
#ifdef SSL_FORBID_ENULL
	*enc |= SSL_eNULL;
#endif
		


	*enc |= (ssl_cipher_methods[SSL_ENC_DES_IDX ] == NULL) ? SSL_DES :0;
	*enc |= (ssl_cipher_methods[SSL_ENC_3DES_IDX] == NULL) ? SSL_3DES:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_RC4_IDX ] == NULL) ? SSL_RC4 :0;
	*enc |= (ssl_cipher_methods[SSL_ENC_RC2_IDX ] == NULL) ? SSL_RC2 :0;
	*enc |= (ssl_cipher_methods[SSL_ENC_IDEA_IDX] == NULL) ? SSL_IDEA:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_AES128_IDX] == NULL) ? SSL_AES128:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_AES256_IDX] == NULL) ? SSL_AES256:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_CAMELLIA128_IDX] == NULL) ? SSL_CAMELLIA128:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_CAMELLIA256_IDX] == NULL) ? SSL_CAMELLIA256:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_GOST89_IDX] == NULL) ? SSL_eGOST2814789CNT:0;
	*enc |= (ssl_cipher_methods[SSL_ENC_SEED_IDX] == NULL) ? SSL_SEED:0;

	*mac |= (ssl_digest_methods[SSL_MD_MD5_IDX ] == NULL) ? SSL_MD5 :0;
	*mac |= (ssl_digest_methods[SSL_MD_SHA1_IDX] == NULL) ? SSL_SHA1:0;
	*mac |= (ssl_digest_methods[SSL_MD_GOST94_IDX] == NULL) ? SSL_GOST94:0;
	*mac |= (ssl_digest_methods[SSL_MD_GOST89MAC_IDX] == NULL || ssl_mac_pkey_id[SSL_MD_GOST89MAC_IDX]==NID_undef)? SSL_GOST89MAC:0;

	}