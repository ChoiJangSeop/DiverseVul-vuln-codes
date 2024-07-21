static SSL_METHOD *ssl23_get_client_method(int ver)
	{
#ifndef OPENSSL_NO_SSL2
	if (ver == SSL2_VERSION)
		return(SSLv2_client_method());
#endif
	if (ver == SSL3_VERSION)
		return(SSLv3_client_method());
	else if (ver == TLS1_VERSION)
		return(TLSv1_client_method());
	else
		return(NULL);
	}