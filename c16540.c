static void ssl_cert_set_default_md(CERT *cert)
	{
	/* Set digest values to defaults */
#ifndef OPENSSL_NO_DSA
	cert->pkeys[SSL_PKEY_DSA_SIGN].digest = EVP_sha1();
#endif
#ifndef OPENSSL_NO_RSA
	cert->pkeys[SSL_PKEY_RSA_SIGN].digest = EVP_sha1();
	cert->pkeys[SSL_PKEY_RSA_ENC].digest = EVP_sha1();
#endif
#ifndef OPENSSL_NO_ECDSA
	cert->pkeys[SSL_PKEY_ECC].digest = EVP_sha1();
#endif
	}