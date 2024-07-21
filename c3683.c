 srtp_protect(srtp_ctx_t *ctx, void *rtp_hdr, int *pkt_octet_len) {
   srtp_hdr_t *hdr = (srtp_hdr_t *)rtp_hdr;
   uint32_t *enc_start;        /* pointer to start of encrypted portion  */
   uint32_t *auth_start;       /* pointer to start of auth. portion      */
   int enc_octet_len = 0; /* number of octets in encrypted portion  */
   xtd_seq_num_t est;          /* estimated xtd_seq_num_t of *hdr        */
   int delta;                  /* delta of local pkt idx and that in hdr */
   uint8_t *auth_tag = NULL;   /* location of auth_tag within packet     */
   err_status_t status;   
   int tag_len;
   srtp_stream_ctx_t *stream;
   int prefix_len;

   debug_print(mod_srtp, "function srtp_protect", NULL);

  /* we assume the hdr is 32-bit aligned to start */

  /* Verify RTP header */
  status = srtp_validate_rtp_header(rtp_hdr, pkt_octet_len);
  if (status)
    return status;

   /* check the packet length - it must at least contain a full header */
   if (*pkt_octet_len < octets_in_rtp_header)
     return err_status_bad_param;

   /*
    * look up ssrc in srtp_stream list, and process the packet with
    * the appropriate stream.  if we haven't seen this stream before,
    * there's a template key for this srtp_session, and the cipher
    * supports key-sharing, then we assume that a new stream using
    * that key has just started up
    */
   stream = srtp_get_stream(ctx, hdr->ssrc);
   if (stream == NULL) {
     if (ctx->stream_template != NULL) {
       srtp_stream_ctx_t *new_stream;

       /* allocate and initialize a new stream */
       status = srtp_stream_clone(ctx->stream_template, 
				  hdr->ssrc, &new_stream); 
       if (status)
	 return status;

       /* add new stream to the head of the stream_list */
       new_stream->next = ctx->stream_list;
       ctx->stream_list = new_stream;

       /* set direction to outbound */
       new_stream->direction = dir_srtp_sender;

       /* set stream (the pointer used in this function) */
       stream = new_stream;
     } else {
       /* no template stream, so we return an error */
       return err_status_no_ctx;
     } 
   }

   /* 
    * verify that stream is for sending traffic - this check will
    * detect SSRC collisions, since a stream that appears in both
    * srtp_protect() and srtp_unprotect() will fail this test in one of
    * those functions.
    */
  if (stream->direction != dir_srtp_sender) {
     if (stream->direction == dir_unknown) {
       stream->direction = dir_srtp_sender;
     } else {
       srtp_handle_event(ctx, stream, event_ssrc_collision);
     }
  }

   /*
    * Check if this is an AEAD stream (GCM mode).  If so, then dispatch
    * the request to our AEAD handler.
    */
  if (stream->rtp_cipher->algorithm == AES_128_GCM ||
      stream->rtp_cipher->algorithm == AES_256_GCM) {
      return srtp_protect_aead(ctx, stream, rtp_hdr, (unsigned int*)pkt_octet_len);
  }

  /* 
   * update the key usage limit, and check it to make sure that we
   * didn't just hit either the soft limit or the hard limit, and call
   * the event handler if we hit either.
   */
  switch(key_limit_update(stream->limit)) {
  case key_event_normal:
    break;
  case key_event_soft_limit: 
    srtp_handle_event(ctx, stream, event_key_soft_limit);
    break; 
  case key_event_hard_limit:
    srtp_handle_event(ctx, stream, event_key_hard_limit);
	return err_status_key_expired;
  default:
    break;
  }

   /* get tag length from stream */
   tag_len = auth_get_tag_length(stream->rtp_auth); 

   /*
    * find starting point for encryption and length of data to be
    * encrypted - the encrypted portion starts after the rtp header
    * extension, if present; otherwise, it starts after the last csrc,
    * if any are present
    *
    * if we're not providing confidentiality, set enc_start to NULL
    */
   if (stream->rtp_services & sec_serv_conf) {
     enc_start = (uint32_t *)hdr + uint32s_in_rtp_header + hdr->cc;  
     if (hdr->x == 1) {
       srtp_hdr_xtnd_t *xtn_hdr = (srtp_hdr_xtnd_t *)enc_start;
       enc_start += (ntohs(xtn_hdr->length) + 1);
     }
     if (!((uint8_t*)enc_start < (uint8_t*)hdr + *pkt_octet_len))
       return err_status_parse_err;
     enc_octet_len = (int)(*pkt_octet_len -
                                    ((uint8_t*)enc_start - (uint8_t*)hdr));
     if (enc_octet_len < 0) return err_status_parse_err;
   } else {
     enc_start = NULL;
   }

   /* 
    * if we're providing authentication, set the auth_start and auth_tag
    * pointers to the proper locations; otherwise, set auth_start to NULL
    * to indicate that no authentication is needed
    */
   if (stream->rtp_services & sec_serv_auth) {
     auth_start = (uint32_t *)hdr;
     auth_tag = (uint8_t *)hdr + *pkt_octet_len;
   } else {
     auth_start = NULL;
     auth_tag = NULL;
   }

   /*
    * estimate the packet index using the start of the replay window   
    * and the sequence number from the header
    */
   delta = rdbx_estimate_index(&stream->rtp_rdbx, &est, ntohs(hdr->seq));
   status = rdbx_check(&stream->rtp_rdbx, delta);
   if (status) {
     if (status != err_status_replay_fail || !stream->allow_repeat_tx)
       return status;  /* we've been asked to reuse an index */
   }
   else
     rdbx_add_index(&stream->rtp_rdbx, delta);

#ifdef NO_64BIT_MATH
   debug_print2(mod_srtp, "estimated packet index: %08x%08x", 
		high32(est),low32(est));
#else
   debug_print(mod_srtp, "estimated packet index: %016llx", est);
#endif

   /* 
    * if we're using rindael counter mode, set nonce and seq 
    */
   if (stream->rtp_cipher->type->id == AES_ICM ||
       stream->rtp_cipher->type->id == AES_256_ICM) {
     v128_t iv;

     iv.v32[0] = 0;
     iv.v32[1] = hdr->ssrc;
#ifdef NO_64BIT_MATH
     iv.v64[1] = be64_to_cpu(make64((high32(est) << 16) | (low32(est) >> 16),
								 low32(est) << 16));
#else
     iv.v64[1] = be64_to_cpu(est << 16);
#endif
     status = cipher_set_iv(stream->rtp_cipher, &iv, direction_encrypt);

   } else {  
     v128_t iv;

     /* otherwise, set the index to est */  
#ifdef NO_64BIT_MATH
     iv.v32[0] = 0;
     iv.v32[1] = 0;
#else
     iv.v64[0] = 0;
#endif
     iv.v64[1] = be64_to_cpu(est);
     status = cipher_set_iv(stream->rtp_cipher, &iv, direction_encrypt);
   }
   if (status)
     return err_status_cipher_fail;

   /* shift est, put into network byte order */
#ifdef NO_64BIT_MATH
   est = be64_to_cpu(make64((high32(est) << 16) |
						 (low32(est) >> 16),
						 low32(est) << 16));
#else
   est = be64_to_cpu(est << 16);
#endif
   
   /* 
    * if we're authenticating using a universal hash, put the keystream
    * prefix into the authentication tag
    */
   if (auth_start) {
     
    prefix_len = auth_get_prefix_length(stream->rtp_auth);    
    if (prefix_len) {
      status = cipher_output(stream->rtp_cipher, auth_tag, prefix_len);
      if (status)
	return err_status_cipher_fail;
      debug_print(mod_srtp, "keystream prefix: %s", 
		  octet_string_hex_string(auth_tag, prefix_len));
    }
  }

  /* if we're encrypting, exor keystream into the message */
  if (enc_start) {
    status = cipher_encrypt(stream->rtp_cipher, 
			    (uint8_t *)enc_start, (unsigned int*)&enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  /*
   *  if we're authenticating, run authentication function and put result
   *  into the auth_tag 
   */
  if (auth_start) {        

    /* initialize auth func context */
    status = auth_start(stream->rtp_auth);
    if (status) return status;

    /* run auth func over packet */
    status = auth_update(stream->rtp_auth, 
			 (uint8_t *)auth_start, *pkt_octet_len);
    if (status) return status;
    
    /* run auth func over ROC, put result into auth_tag */
    debug_print(mod_srtp, "estimated packet index: %016llx", est);
    status = auth_compute(stream->rtp_auth, (uint8_t *)&est, 4, auth_tag); 
    debug_print(mod_srtp, "srtp auth tag:    %s", 
		octet_string_hex_string(auth_tag, tag_len));
    if (status)
      return err_status_auth_fail;   

  }

  if (auth_tag) {

    /* increase the packet length by the length of the auth tag */
    *pkt_octet_len += tag_len;
  }

  return err_status_ok;  
}