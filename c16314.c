unsigned char *ssl_add_clienthello_tlsext(SSL *s, unsigned char *p, unsigned char *limit)
	{
	int extdatalen=0;
	unsigned char *ret = p;

	/* don't add extensions for SSLv3 unless doing secure renegotiation */
	if (s->client_version == SSL3_VERSION
					&& !s->s3->send_connection_binding)
		return p;

	ret+=2;

	if (ret>=limit) return NULL; /* this really never occurs, but ... */

 	if (s->tlsext_hostname != NULL)
		{ 
		/* Add TLS extension servername to the Client Hello message */
		unsigned long size_str;
		long lenmax; 

		/* check for enough space.
		   4 for the servername type and entension length
		   2 for servernamelist length
		   1 for the hostname type
		   2 for hostname length
		   + hostname length 
		*/
		   
		if ((lenmax = limit - ret - 9) < 0 
		    || (size_str = strlen(s->tlsext_hostname)) > (unsigned long)lenmax) 
			return NULL;
			
		/* extension type and length */
		s2n(TLSEXT_TYPE_server_name,ret); 
		s2n(size_str+5,ret);
		
		/* length of servername list */
		s2n(size_str+3,ret);
	
		/* hostname type, length and hostname */
		*(ret++) = (unsigned char) TLSEXT_NAMETYPE_host_name;
		s2n(size_str,ret);
		memcpy(ret, s->tlsext_hostname, size_str);
		ret+=size_str;
		}

        /* Add RI if renegotiating */
        if (s->renegotiate)
          {
          int el;
          
          if(!ssl_add_clienthello_renegotiate_ext(s, 0, &el, 0))
              {
              SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
              return NULL;
              }

          if((limit - p - 4 - el) < 0) return NULL;
          
          s2n(TLSEXT_TYPE_renegotiate,ret);
          s2n(el,ret);

          if(!ssl_add_clienthello_renegotiate_ext(s, ret, &el, el))
              {
              SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
              return NULL;
              }

          ret += el;
        }

#ifndef OPENSSL_NO_SRP
#define MIN(x,y) (((x)<(y))?(x):(y))
	/* we add SRP username the first time only if we have one! */
	if (s->srp_ctx.login != NULL)
		{/* Add TLS extension SRP username to the Client Hello message */
		int login_len = MIN(strlen(s->srp_ctx.login) + 1, 255);
		long lenmax; 

		if ((lenmax = limit - ret - 5) < 0) return NULL; 
		if (login_len > lenmax) return NULL;
		if (login_len > 255)
			{
			SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
			return NULL;
			}
		s2n(TLSEXT_TYPE_srp,ret);
		s2n(login_len+1,ret);

		(*ret++) = (unsigned char) MIN(strlen(s->srp_ctx.login), 254);
		memcpy(ret, s->srp_ctx.login, MIN(strlen(s->srp_ctx.login), 254));
		ret+=login_len;
		}
#endif

#ifndef OPENSSL_NO_EC
	if (s->tlsext_ecpointformatlist != NULL &&
	    s->version != DTLS1_VERSION)
		{
		/* Add TLS extension ECPointFormats to the ClientHello message */
		long lenmax; 

		if ((lenmax = limit - ret - 5) < 0) return NULL; 
		if (s->tlsext_ecpointformatlist_length > (unsigned long)lenmax) return NULL;
		if (s->tlsext_ecpointformatlist_length > 255)
			{
			SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
			return NULL;
			}
		
		s2n(TLSEXT_TYPE_ec_point_formats,ret);
		s2n(s->tlsext_ecpointformatlist_length + 1,ret);
		*(ret++) = (unsigned char) s->tlsext_ecpointformatlist_length;
		memcpy(ret, s->tlsext_ecpointformatlist, s->tlsext_ecpointformatlist_length);
		ret+=s->tlsext_ecpointformatlist_length;
		}
	if (s->tlsext_ellipticcurvelist != NULL &&
	    s->version != DTLS1_VERSION)
		{
		/* Add TLS extension EllipticCurves to the ClientHello message */
		long lenmax; 

		if ((lenmax = limit - ret - 6) < 0) return NULL; 
		if (s->tlsext_ellipticcurvelist_length > (unsigned long)lenmax) return NULL;
		if (s->tlsext_ellipticcurvelist_length > 65532)
			{
			SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
			return NULL;
			}
		
		s2n(TLSEXT_TYPE_elliptic_curves,ret);
		s2n(s->tlsext_ellipticcurvelist_length + 2, ret);

		/* NB: draft-ietf-tls-ecc-12.txt uses a one-byte prefix for
		 * elliptic_curve_list, but the examples use two bytes.
		 * http://www1.ietf.org/mail-archive/web/tls/current/msg00538.html
		 * resolves this to two bytes.
		 */
		s2n(s->tlsext_ellipticcurvelist_length, ret);
		memcpy(ret, s->tlsext_ellipticcurvelist, s->tlsext_ellipticcurvelist_length);
		ret+=s->tlsext_ellipticcurvelist_length;
		}
#endif /* OPENSSL_NO_EC */

	if (!(SSL_get_options(s) & SSL_OP_NO_TICKET))
		{
		int ticklen;
		if (!s->new_session && s->session && s->session->tlsext_tick)
			ticklen = s->session->tlsext_ticklen;
		else if (s->session && s->tlsext_session_ticket &&
			 s->tlsext_session_ticket->data)
			{
			ticklen = s->tlsext_session_ticket->length;
			s->session->tlsext_tick = OPENSSL_malloc(ticklen);
			if (!s->session->tlsext_tick)
				return NULL;
			memcpy(s->session->tlsext_tick,
			       s->tlsext_session_ticket->data,
			       ticklen);
			s->session->tlsext_ticklen = ticklen;
			}
		else
			ticklen = 0;
		if (ticklen == 0 && s->tlsext_session_ticket &&
		    s->tlsext_session_ticket->data == NULL)
			goto skip_ext;
		/* Check for enough room 2 for extension type, 2 for len
 		 * rest for ticket
  		 */
		if ((long)(limit - ret - 4 - ticklen) < 0) return NULL;
		s2n(TLSEXT_TYPE_session_ticket,ret); 
		s2n(ticklen,ret);
		if (ticklen)
			{
			memcpy(ret, s->session->tlsext_tick, ticklen);
			ret += ticklen;
			}
		}
		skip_ext:

	if (TLS1_get_version(s) >= TLS1_2_VERSION)
		{
		if ((size_t)(limit - ret) < sizeof(tls12_sigalgs) + 6)
			return NULL; 
		s2n(TLSEXT_TYPE_signature_algorithms,ret);
		s2n(sizeof(tls12_sigalgs) + 2, ret);
		s2n(sizeof(tls12_sigalgs), ret);
		memcpy(ret, tls12_sigalgs, sizeof(tls12_sigalgs));
		ret += sizeof(tls12_sigalgs);
		}

#ifdef TLSEXT_TYPE_opaque_prf_input
	if (s->s3->client_opaque_prf_input != NULL &&
	    s->version != DTLS1_VERSION)
		{
		size_t col = s->s3->client_opaque_prf_input_len;
		
		if ((long)(limit - ret - 6 - col < 0))
			return NULL;
		if (col > 0xFFFD) /* can't happen */
			return NULL;

		s2n(TLSEXT_TYPE_opaque_prf_input, ret); 
		s2n(col + 2, ret);
		s2n(col, ret);
		memcpy(ret, s->s3->client_opaque_prf_input, col);
		ret += col;
		}
#endif

	if (s->tlsext_status_type == TLSEXT_STATUSTYPE_ocsp &&
	    s->version != DTLS1_VERSION)
		{
		int i;
		long extlen, idlen, itmp;
		OCSP_RESPID *id;

		idlen = 0;
		for (i = 0; i < sk_OCSP_RESPID_num(s->tlsext_ocsp_ids); i++)
			{
			id = sk_OCSP_RESPID_value(s->tlsext_ocsp_ids, i);
			itmp = i2d_OCSP_RESPID(id, NULL);
			if (itmp <= 0)
				return NULL;
			idlen += itmp + 2;
			}

		if (s->tlsext_ocsp_exts)
			{
			extlen = i2d_X509_EXTENSIONS(s->tlsext_ocsp_exts, NULL);
			if (extlen < 0)
				return NULL;
			}
		else
			extlen = 0;
			
		if ((long)(limit - ret - 7 - extlen - idlen) < 0) return NULL;
		s2n(TLSEXT_TYPE_status_request, ret);
		if (extlen + idlen > 0xFFF0)
			return NULL;
		s2n(extlen + idlen + 5, ret);
		*(ret++) = TLSEXT_STATUSTYPE_ocsp;
		s2n(idlen, ret);
		for (i = 0; i < sk_OCSP_RESPID_num(s->tlsext_ocsp_ids); i++)
			{
			/* save position of id len */
			unsigned char *q = ret;
			id = sk_OCSP_RESPID_value(s->tlsext_ocsp_ids, i);
			/* skip over id len */
			ret += 2;
			itmp = i2d_OCSP_RESPID(id, &ret);
			/* write id len */
			s2n(itmp, q);
			}
		s2n(extlen, ret);
		if (extlen > 0)
			i2d_X509_EXTENSIONS(s->tlsext_ocsp_exts, &ret);
		}

#ifndef OPENSSL_NO_NEXTPROTONEG
	if (s->ctx->next_proto_select_cb && !s->s3->tmp.finish_md_len)
		{
		/* The client advertises an emtpy extension to indicate its
		 * support for Next Protocol Negotiation */
		if (limit - ret - 4 < 0)
			return NULL;
		s2n(TLSEXT_TYPE_next_proto_neg,ret);
		s2n(0,ret);
		}
#endif

        if(SSL_get_srtp_profiles(s))
                {
                int el;

                ssl_add_clienthello_use_srtp_ext(s, 0, &el, 0);
                
                if((limit - p - 4 - el) < 0) return NULL;

                s2n(TLSEXT_TYPE_use_srtp,ret);
                s2n(el,ret);

                if(ssl_add_clienthello_use_srtp_ext(s, ret, &el, el))
			{
			SSLerr(SSL_F_SSL_ADD_CLIENTHELLO_TLSEXT, ERR_R_INTERNAL_ERROR);
			return NULL;
			}
                ret += el;
                }

	if ((extdatalen = ret-p-2)== 0) 
		return p;

	s2n(extdatalen,p);
	return ret;
	}