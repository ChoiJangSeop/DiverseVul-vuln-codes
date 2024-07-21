srtp_unprotect_aead (srtp_ctx_t *ctx, srtp_stream_ctx_t *stream, int delta, 
	             xtd_seq_num_t est, void *srtp_hdr, unsigned int *pkt_octet_len)
{
    srtp_hdr_t *hdr = (srtp_hdr_t*)srtp_hdr;
    uint32_t *enc_start;        /* pointer to start of encrypted portion  */
    unsigned int enc_octet_len = 0; /* number of octets in encrypted portion */
    v128_t iv;
    err_status_t status;
    int tag_len;
    unsigned int aad_len;

    debug_print(mod_srtp, "function srtp_unprotect_aead", NULL);

#ifdef NO_64BIT_MATH
    debug_print2(mod_srtp, "estimated u_packet index: %08x%08x", high32(est), low32(est));
#else
    debug_print(mod_srtp, "estimated u_packet index: %016llx", est);
#endif

    /* get tag length from stream */
    tag_len = auth_get_tag_length(stream->rtp_auth);

    /*
     * AEAD uses a new IV formation method 
     */
    srtp_calc_aead_iv(stream, &iv, &est, hdr);
    status = cipher_set_iv(stream->rtp_cipher, &iv, direction_decrypt);
    if (status) {
        return err_status_cipher_fail;
    }

    /*
     * find starting point for decryption and length of data to be
     * decrypted - the encrypted portion starts after the rtp header
     * extension, if present; otherwise, it starts after the last csrc,
     * if any are present
     */
    enc_start = (uint32_t*)hdr + uint32s_in_rtp_header + hdr->cc;
    if (hdr->x == 1) {
        srtp_hdr_xtnd_t *xtn_hdr = (srtp_hdr_xtnd_t*)enc_start;
        enc_start += (ntohs(xtn_hdr->length) + 1);
    }
    if (!((uint8_t*)enc_start < (uint8_t*)hdr + *pkt_octet_len))
        return err_status_parse_err;
    /*
     * We pass the tag down to the cipher when doing GCM mode 
     */
    enc_octet_len = (unsigned int)(*pkt_octet_len - 
                                   ((uint8_t*)enc_start - (uint8_t*)hdr));

    /*
     * Sanity check the encrypted payload length against
     * the tag size.  It must always be at least as large
     * as the tag length.
     */
    if (enc_octet_len < (unsigned int) tag_len) {
        return err_status_cipher_fail;
    }

    /*
     * update the key usage limit, and check it to make sure that we
     * didn't just hit either the soft limit or the hard limit, and call
     * the event handler if we hit either.
     */
    switch (key_limit_update(stream->limit)) {
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

    /*
     * Set the AAD for AES-GCM, which is the RTP header
     */
    aad_len = (uint8_t *)enc_start - (uint8_t *)hdr;
    status = cipher_set_aad(stream->rtp_cipher, (uint8_t*)hdr, aad_len);
    if (status) {
        return ( err_status_cipher_fail);
    }

    /* Decrypt the ciphertext.  This also checks the auth tag based 
     * on the AAD we just specified above */
    status = cipher_decrypt(stream->rtp_cipher,
                            (uint8_t*)enc_start, &enc_octet_len);
    if (status) {
        return status;
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
        if (status) {
            return status;
        }

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