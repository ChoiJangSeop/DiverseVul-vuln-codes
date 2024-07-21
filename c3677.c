srtp_unprotect(srtp_ctx_t *ctx, void *srtp_hdr, int *pkt_octet_len) {
  srtp_hdr_t *hdr = (srtp_hdr_t *)srtp_hdr;
  uint32_t *enc_start;      /* pointer to start of encrypted portion  */
  uint32_t *auth_start;     /* pointer to start of auth. portion      */
  unsigned int enc_octet_len = 0;/* number of octets in encrypted portion */
  uint8_t *auth_tag = NULL; /* location of auth_tag within packet     */
  xtd_seq_num_t est;        /* estimated xtd_seq_num_t of *hdr        */
  int delta;                /* delta of local pkt idx and that in hdr */
  v128_t iv;
  err_status_t status;
  srtp_stream_ctx_t *stream;
  uint8_t tmp_tag[SRTP_MAX_TAG_LEN];
  int tag_len, prefix_len;

  debug_print(mod_srtp, "function srtp_unprotect", NULL);

  /* we assume the hdr is 32-bit aligned to start */

  /* Verify RTP header */
  status = srtp_validate_rtp_header(srtp_hdr, pkt_octet_len);
  if (status)
    return status;

  /* check the packet length - it must at least contain a full header */
  if (*pkt_octet_len < octets_in_rtp_header)
    return err_status_bad_param;

  /*
   * look up ssrc in srtp_stream list, and process the packet with 
   * the appropriate stream.  if we haven't seen this stream before,
   * there's only one key for this srtp_session, and the cipher
   * supports key-sharing, then we assume that a new stream using
   * that key has just started up
   */
  stream = srtp_get_stream(ctx, hdr->ssrc);
  if (stream == NULL) {
    if (ctx->stream_template != NULL) {
      stream = ctx->stream_template;
      debug_print(mod_srtp, "using provisional stream (SSRC: 0x%08x)",
		  hdr->ssrc);
      
      /* 
       * set estimated packet index to sequence number from header,
       * and set delta equal to the same value
       */
#ifdef NO_64BIT_MATH
      est = (xtd_seq_num_t) make64(0,ntohs(hdr->seq));
      delta = low32(est);
#else
      est = (xtd_seq_num_t) ntohs(hdr->seq);
      delta = (int)est;
#endif
    } else {
      
      /*
       * no stream corresponding to SSRC found, and we don't do
       * key-sharing, so return an error
       */
      return err_status_no_ctx;
    }
  } else {
  
    /* estimate packet index from seq. num. in header */
    delta = rdbx_estimate_index(&stream->rtp_rdbx, &est, ntohs(hdr->seq));
    
    /* check replay database */
    status = rdbx_check(&stream->rtp_rdbx, delta);
    if (status)
      return status;
  }

#ifdef NO_64BIT_MATH
  debug_print2(mod_srtp, "estimated u_packet index: %08x%08x", high32(est),low32(est));
#else
  debug_print(mod_srtp, "estimated u_packet index: %016llx", est);
#endif

  /*
   * Check if this is an AEAD stream (GCM mode).  If so, then dispatch
   * the request to our AEAD handler.
   */
  if (stream->rtp_cipher->algorithm == AES_128_GCM ||
      stream->rtp_cipher->algorithm == AES_256_GCM) {
      return srtp_unprotect_aead(ctx, stream, delta, est, srtp_hdr, (unsigned int*)pkt_octet_len);
  }

  /* get tag length from stream */
  tag_len = auth_get_tag_length(stream->rtp_auth); 

  /* 
   * set the cipher's IV properly, depending on whatever cipher we
   * happen to be using
   */
  if (stream->rtp_cipher->type->id == AES_ICM ||
      stream->rtp_cipher->type->id == AES_256_ICM) {

    /* aes counter mode */
    iv.v32[0] = 0;
    iv.v32[1] = hdr->ssrc;  /* still in network order */
#ifdef NO_64BIT_MATH
    iv.v64[1] = be64_to_cpu(make64((high32(est) << 16) | (low32(est) >> 16),
			         low32(est) << 16));
#else
    iv.v64[1] = be64_to_cpu(est << 16);
#endif
    status = cipher_set_iv(stream->rtp_cipher, &iv, direction_decrypt);
  } else {  
    
    /* no particular format - set the iv to the pakcet index */  
#ifdef NO_64BIT_MATH
    iv.v32[0] = 0;
    iv.v32[1] = 0;
#else
    iv.v64[0] = 0;
#endif
    iv.v64[1] = be64_to_cpu(est);
    status = cipher_set_iv(stream->rtp_cipher, &iv, direction_decrypt);
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
   * find starting point for decryption and length of data to be
   * decrypted - the encrypted portion starts after the rtp header
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
    enc_octet_len = (uint32_t)(*pkt_octet_len - tag_len -
                               ((uint8_t*)enc_start - (uint8_t*)hdr));
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
    auth_tag = (uint8_t *)hdr + *pkt_octet_len - tag_len;
  } else {
    auth_start = NULL;
    auth_tag = NULL;
  } 

  /*
   * if we expect message authentication, run the authentication
   * function and compare the result with the value of the auth_tag
   */
  if (auth_start) {        

    /* 
     * if we're using a universal hash, then we need to compute the
     * keystream prefix for encrypting the universal hash output
     *
     * if the keystream prefix length is zero, then we know that
     * the authenticator isn't using a universal hash function
     */  
    if (stream->rtp_auth->prefix_len != 0) {
      
      prefix_len = auth_get_prefix_length(stream->rtp_auth);    
      status = cipher_output(stream->rtp_cipher, tmp_tag, prefix_len);
      debug_print(mod_srtp, "keystream prefix: %s", 
		  octet_string_hex_string(tmp_tag, prefix_len));
      if (status)
	return err_status_cipher_fail;
    } 

    /* initialize auth func context */
    status = auth_start(stream->rtp_auth);
    if (status) return status;
 
    /* now compute auth function over packet */
    status = auth_update(stream->rtp_auth, (uint8_t *)auth_start,  
			 *pkt_octet_len - tag_len);

    /* run auth func over ROC, then write tmp tag */
    status = auth_compute(stream->rtp_auth, (uint8_t *)&est, 4, tmp_tag);  

    debug_print(mod_srtp, "computed auth tag:    %s", 
		octet_string_hex_string(tmp_tag, tag_len));
    debug_print(mod_srtp, "packet auth tag:      %s", 
		octet_string_hex_string(auth_tag, tag_len));
    if (status)
      return err_status_auth_fail;   

    if (octet_string_is_eq(tmp_tag, auth_tag, tag_len))
      return err_status_auth_fail;
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

  /* if we're decrypting, add keystream into ciphertext */
  if (enc_start) {
    status = cipher_decrypt(stream->rtp_cipher, 
			    (uint8_t *)enc_start, &enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  /* 
   * verify that stream is for received traffic - this check will
   * detect SSRC collisions, since a stream that appears in both
   * srtp_protect() and srtp_unprotect() will fail this test in one of
   * those functions.
   *
   * we do this check *after* the authentication check, so that the
   * latter check will catch any attempts to fool us into thinking
   * that we've got a collision
   */
  if (stream->direction != dir_srtp_receiver) {
    if (stream->direction == dir_unknown) {
      stream->direction = dir_srtp_receiver;
    } else {
      srtp_handle_event(ctx, stream, event_ssrc_collision);
    }
  }

  /* 
   * if the stream is a 'provisional' one, in which the template context
   * is used, then we need to allocate a new stream at this point, since
   * the authentication passed
   */
  if (stream == ctx->stream_template) {  
    srtp_stream_ctx_t *new_stream;

    /* 
     * allocate and initialize a new stream 
     * 
     * note that we indicate failure if we can't allocate the new
     * stream, and some implementations will want to not return
     * failure here
     */
    status = srtp_stream_clone(ctx->stream_template, hdr->ssrc, &new_stream); 
    if (status)
      return status;
    
    /* add new stream to the head of the stream_list */
    new_stream->next = ctx->stream_list;
    ctx->stream_list = new_stream;
    
    /* set stream (the pointer used in this function) */
    stream = new_stream;
  }
  
  /* 
   * the message authentication function passed, so add the packet
   * index into the replay database 
   */
  rdbx_add_index(&stream->rtp_rdbx, delta);

  /* decrease the packet length by the length of the auth tag */
  *pkt_octet_len -= tag_len;

  return err_status_ok;  
}