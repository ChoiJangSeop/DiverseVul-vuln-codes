int ssl_parse_clienthello_tlsext(SSL *s, unsigned char **p, unsigned char *d, int n, int *al)
	{
	unsigned short type;
	unsigned short size;
	unsigned short len;
	unsigned char *data = *p;
#if 0
	fprintf(stderr,"ssl_parse_clienthello_tlsext %s\n",s->session->tlsext_hostname?s->session->tlsext_hostname:"NULL");
#endif
	s->servername_done = 0;

	if (data >= (d+n-2))
		return 1;
	n2s(data,len);

        if (data > (d+n-len)) 
		return 1;

	while (data <= (d+n-4))
		{
		n2s(data,type);
		n2s(data,size);

		if (data+size > (d+n))
	   		return 1;
		
/* The servername extension is treated as follows:

   - Only the hostname type is supported with a maximum length of 255.
   - The servername is rejected if too long or if it contains zeros,
     in which case an fatal alert is generated.
   - The servername field is maintained together with the session cache.
   - When a session is resumed, the servername call back invoked in order
     to allow the application to position itself to the right context. 
   - The servername is acknowledged if it is new for a session or when 
     it is identical to a previously used for the same session. 
     Applications can control the behaviour.  They can at any time
     set a 'desirable' servername for a new SSL object. This can be the
     case for example with HTTPS when a Host: header field is received and
     a renegotiation is requested. In this case, a possible servername
     presented in the new client hello is only acknowledged if it matches
     the value of the Host: field. 
   - Applications must  use SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
     if they provide for changing an explicit servername context for the session,
     i.e. when the session has been established with a servername extension. 
   - On session reconnect, the servername extension may be absent. 

*/      

		if (type == TLSEXT_TYPE_server_name)
			{
			unsigned char *sdata = data;
			int servname_type;
			int dsize = size-3 ;
                        
			if (dsize > 0 )
				{
 				servname_type = *(sdata++); 
				n2s(sdata,len);
				if (len != dsize) 
					{
					*al = SSL_AD_DECODE_ERROR;
					return 0;
					}

				switch (servname_type)
					{
				case TLSEXT_NAMETYPE_host_name:
                                        if (s->session->tlsext_hostname == NULL)
						{
						if (len > TLSEXT_MAXLEN_host_name || 
							((s->session->tlsext_hostname = OPENSSL_malloc(len+1)) == NULL))
							{
							*al = TLS1_AD_UNRECOGNIZED_NAME;
							return 0;
							}
						memcpy(s->session->tlsext_hostname, sdata, len);
						s->session->tlsext_hostname[len]='\0';
						if (strlen(s->session->tlsext_hostname) != len) {
							OPENSSL_free(s->session->tlsext_hostname);
							*al = TLS1_AD_UNRECOGNIZED_NAME;
							return 0;
						}
						s->servername_done = 1; 

#if 0
						fprintf(stderr,"ssl_parse_clienthello_tlsext s->session->tlsext_hostname %s\n",s->session->tlsext_hostname);
#endif
						}
					else 
						s->servername_done = strlen(s->session->tlsext_hostname) == len 
							&& strncmp(s->session->tlsext_hostname, (char *)sdata, len) == 0;
					
					break;

				default:
					break;
					}
                                 
				}
			}

		data+=size;		
		}

	*p = data;
	return 1;
}