srtp_protect_aead (srtp_ctx_t *ctx, srtp_stream_ctx_t *stream, 
	           void *rtp_hdr, unsigned int *pkt_octet_len)
{
    srtp_hdr_t *hdr = (srtp_hdr_t*)rtp_hdr;
    uint32_t *enc_start;        /* pointer to start of encrypted portion  */
    unsigned int enc_octet_len = 0; /* number of octets in encrypted portion  */
    xtd_seq_num_t est;          /* estimated xtd_seq_num_t of *hdr        */
    int delta;                  /* delta of local pkt idx and that in hdr */
    err_status_t status;
    int tag_len;
    v128_t iv;
    unsigned int aad_len;

    debug_print(mod_srtp, "function srtp_protect_aead", NULL);

    /*
     * update the key usage limit, and check it to make sure that we
     * didn't just hit either the soft limit or the hard limit, and call
     * the event handler if we hit either.
     */
    switch (key_limit_update(stream->limit)) {
    case key_event_normal:
        break;
    case key_event_hard_limit:
        srtp_handle_event(ctx, stream, event_key_hard_limit);
        return err_status_key_expired;
    case key_event_soft_limit:
    default:
        srtp_handle_event(ctx, stream, event_key_soft_limit);
        break;
    }

    /* get tag length from stream */
    tag_len = auth_get_tag_length(stream->rtp_auth);

    /*
     * find starting point for encryption and length of data to be
     * encrypted - the encrypted portion starts after the rtp header
     * extension, if present; otherwise, it starts after the last csrc,
     * if any are present
     */
     enc_start = (uint32_t*)hdr + uint32s_in_rtp_header + hdr->cc;
     if (hdr->x == 1) {
         srtp_hdr_xtnd_t *xtn_hdr = (srtp_hdr_xtnd_t*)enc_start;
         enc_start += (ntohs(xtn_hdr->length) + 1);
     }
     if (!((uint8_t*)enc_start < (uint8_t*)hdr + (*pkt_octet_len - tag_len)))
         return err_status_parse_err;
     enc_octet_len = (unsigned int)(*pkt_octet_len -
                                    ((uint8_t*)enc_start - (uint8_t*)hdr));

    /*
     * estimate the packet index using the start of the replay window
     * and the sequence number from the header
     */
    delta = rdbx_estimate_index(&stream->rtp_rdbx, &est, ntohs(hdr->seq));
    status = rdbx_check(&stream->rtp_rdbx, delta);
    if (status) {
	if (status != err_status_replay_fail || !stream->allow_repeat_tx) {
	    return status;  /* we've been asked to reuse an index */
	}
    } else {
	rdbx_add_index(&stream->rtp_rdbx, delta);
    }

#ifdef NO_64BIT_MATH
    debug_print2(mod_srtp, "estimated packet index: %08x%08x",
                 high32(est), low32(est));
#else
    debug_print(mod_srtp, "estimated packet index: %016llx", est);
#endif

    /*
     * AEAD uses a new IV formation method
     */
    srtp_calc_aead_iv(stream, &iv, &est, hdr);
    status = cipher_set_iv(stream->rtp_cipher, &iv, direction_encrypt);
    if (status) {
        return err_status_cipher_fail;
    }

    /* shift est, put into network byte order */
#ifdef NO_64BIT_MATH
    est = be64_to_cpu(make64((high32(est) << 16) |
                             (low32(est) >> 16),
                             low32(est) << 16));
#else
    est = be64_to_cpu(est << 16);
#endif

    /*
     * Set the AAD over the RTP header 
     */
    aad_len = (uint8_t *)enc_start - (uint8_t *)hdr;
    status = cipher_set_aad(stream->rtp_cipher, (uint8_t*)hdr, aad_len);
    if (status) {
        return ( err_status_cipher_fail);
    }

    /* Encrypt the payload  */
    status = cipher_encrypt(stream->rtp_cipher,
                            (uint8_t*)enc_start, &enc_octet_len);
    if (status) {
        return err_status_cipher_fail;
    }
    /*
     * If we're doing GCM, we need to get the tag
     * and append that to the output
     */
    status = cipher_get_tag(stream->rtp_cipher, 
                            (uint8_t*)enc_start+enc_octet_len, &tag_len);
    if (status) {
	return ( err_status_cipher_fail);
    }
    enc_octet_len += tag_len;

    /* increase the packet length by the length of the auth tag */
    *pkt_octet_len += tag_len;

    return err_status_ok;
}