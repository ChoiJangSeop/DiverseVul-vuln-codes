unsigned char *ssl_add_serverhello_tlsext(SSL *s, unsigned char *p, unsigned char *limit)
	{
	int extdatalen=0;
	unsigned char *ret = p;

	ret+=2;
	if (ret>=limit) return NULL; /* this really never occurs, but ... */

	if (!s->hit && s->servername_done == 1 && s->session->tlsext_hostname != NULL)
		{ 
		if (limit - p - 4 < 0) return NULL; 

		s2n(TLSEXT_TYPE_server_name,ret);
		s2n(0,ret);
		}
	
	if ((extdatalen = ret-p-2)== 0) 
		return p;

	s2n(extdatalen,p);
	return ret;
}