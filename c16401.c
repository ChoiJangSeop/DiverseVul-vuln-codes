unsigned char *ssl_add_clienthello_tlsext(SSL *s, unsigned char *p, unsigned char *limit)
	{
	int extdatalen=0;
	unsigned char *ret = p;

	ret+=2;

	if (ret>=limit) return NULL; /* this really never occurs, but ... */
 	if (s->servername_done == 0 && s->tlsext_hostname != NULL)
		{ 
		/* Add TLS extension servername to the Client Hello message */
		unsigned long size_str;
		long lenmax; 

		if ((lenmax = limit - p - 7) < 0) return NULL; 
		if ((size_str = strlen(s->tlsext_hostname)) > (unsigned long)lenmax) return NULL;
		
		s2n(TLSEXT_TYPE_server_name,ret);
		s2n(size_str+3,ret);
		*(ret++) = (unsigned char) TLSEXT_NAMETYPE_host_name;
		s2n(size_str,ret);
	
		memcpy(ret, s->tlsext_hostname, size_str);
		ret+=size_str;
		}

	if ((extdatalen = ret-p-2)== 0) 
		return p;

	s2n(extdatalen,p);
	return ret;
}