unsigned char *ssl_add_serverhello_tlsext(SSL *s, unsigned char *p, unsigned char *limit)
	{
	int extdatalen=0;
	unsigned char *ret = p;

	/* don't add extensions for SSLv3, unless doing secure renegotiation */
	if (s->version == SSL3_VERSION && !s->s3->send_connection_binding)
		return p;
	
	ret+=2;
	if (ret>=limit) return NULL; /* this really never occurs, but ... */

	if (!s->hit && s->servername_done == 1 && s->session->tlsext_hostname != NULL)
		{ 
		if ((long)(limit - ret - 4) < 0) return NULL; 

		s2n(TLSEXT_TYPE_server_name,ret);
		s2n(0,ret);
		}

	if(s->s3->send_connection_binding)
        {
          int el;
          
          if(!ssl_add_serverhello_renegotiate_ext(s, 0, &el, 0))
              {
              SSLerr(SSL_F_SSL_ADD_SERVERHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
              return NULL;
              }

          if((limit - p - 4 - el) < 0) return NULL;
          
          s2n(TLSEXT_TYPE_renegotiate,ret);
          s2n(el,ret);

          if(!ssl_add_serverhello_renegotiate_ext(s, ret, &el, el))
              {
              SSLerr(SSL_F_SSL_ADD_SERVERHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
              return NULL;
              }

          ret += el;
        }

#ifndef OPENSSL_NO_EC
	if (s->tlsext_ecpointformatlist != NULL &&
	    s->version != DTLS1_VERSION)
		{
		/* Add TLS extension ECPointFormats to the ServerHello message */
		long lenmax; 

		if ((lenmax = limit - ret - 5) < 0) return NULL; 
		if (s->tlsext_ecpointformatlist_length > (unsigned long)lenmax) return NULL;
		if (s->tlsext_ecpointformatlist_length > 255)
			{
			SSLerr(SSL_F_SSL_ADD_SERVERHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
			return NULL;
			}
		
		s2n(TLSEXT_TYPE_ec_point_formats,ret);
		s2n(s->tlsext_ecpointformatlist_length + 1,ret);
		*(ret++) = (unsigned char) s->tlsext_ecpointformatlist_length;
		memcpy(ret, s->tlsext_ecpointformatlist, s->tlsext_ecpointformatlist_length);
		ret+=s->tlsext_ecpointformatlist_length;

		}
	/* Currently the server should not respond with a SupportedCurves extension */
#endif /* OPENSSL_NO_EC */

	if (s->tlsext_ticket_expected
		&& !(SSL_get_options(s) & SSL_OP_NO_TICKET)) 
		{ 
		if ((long)(limit - ret - 4) < 0) return NULL; 
		s2n(TLSEXT_TYPE_session_ticket,ret);
		s2n(0,ret);
		}

	if (s->tlsext_status_expected)
		{ 
		if ((long)(limit - ret - 4) < 0) return NULL; 
		s2n(TLSEXT_TYPE_status_request,ret);
		s2n(0,ret);
		}

#ifdef TLSEXT_TYPE_opaque_prf_input
	if (s->s3->server_opaque_prf_input != NULL &&
	    s->version != DTLS1_VERSION)
		{
		size_t sol = s->s3->server_opaque_prf_input_len;
		
		if ((long)(limit - ret - 6 - sol) < 0)
			return NULL;
		if (sol > 0xFFFD) /* can't happen */
			return NULL;

		s2n(TLSEXT_TYPE_opaque_prf_input, ret); 
		s2n(sol + 2, ret);
		s2n(sol, ret);
		memcpy(ret, s->s3->server_opaque_prf_input, sol);
		ret += sol;
		}
#endif
	if (((s->s3->tmp.new_cipher->id & 0xFFFF)==0x80 || (s->s3->tmp.new_cipher->id & 0xFFFF)==0x81) 
		&& (SSL_get_options(s) & SSL_OP_CRYPTOPRO_TLSEXT_BUG))
		{ const unsigned char cryptopro_ext[36] = {
			0xfd, 0xe8, /*65000*/
			0x00, 0x20, /*32 bytes length*/
			0x30, 0x1e, 0x30, 0x08, 0x06, 0x06, 0x2a, 0x85, 
			0x03,   0x02, 0x02, 0x09, 0x30, 0x08, 0x06, 0x06, 
			0x2a, 0x85, 0x03, 0x02, 0x02, 0x16, 0x30, 0x08, 
			0x06, 0x06, 0x2a, 0x85, 0x03, 0x02, 0x02, 0x17};
			if (limit-ret<36) return NULL;
			memcpy(ret,cryptopro_ext,36);
			ret+=36;

		}

	if ((extdatalen = ret-p-2)== 0) 
		return p;

	s2n(extdatalen,p);
	return ret;
	}