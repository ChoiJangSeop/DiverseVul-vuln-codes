static int dtls1_preprocess_fragment(SSL *s, struct hm_header_st *msg_hdr)
{
    size_t frag_off, frag_len, msg_len;

    msg_len = msg_hdr->msg_len;
    frag_off = msg_hdr->frag_off;
    frag_len = msg_hdr->frag_len;

    /* sanity checking */
    if ((frag_off + frag_len) > msg_len) {
        SSLerr(SSL_F_DTLS1_PREPROCESS_FRAGMENT, SSL_R_EXCESSIVE_MESSAGE_SIZE);
        return SSL_AD_ILLEGAL_PARAMETER;
    }

    if (s->d1->r_msg_hdr.frag_off == 0) { /* first fragment */
        /*
         * msg_len is limited to 2^24, but is effectively checked against max
         * above
         */
        if (!BUF_MEM_grow_clean(s->init_buf, msg_len + DTLS1_HM_HEADER_LENGTH)) {
            SSLerr(SSL_F_DTLS1_PREPROCESS_FRAGMENT, ERR_R_BUF_LIB);
            return SSL_AD_INTERNAL_ERROR;
        }

        s->s3->tmp.message_size = msg_len;
        s->d1->r_msg_hdr.msg_len = msg_len;
        s->s3->tmp.message_type = msg_hdr->type;
        s->d1->r_msg_hdr.type = msg_hdr->type;
        s->d1->r_msg_hdr.seq = msg_hdr->seq;
    } else if (msg_len != s->d1->r_msg_hdr.msg_len) {
        /*
         * They must be playing with us! BTW, failure to enforce upper limit
         * would open possibility for buffer overrun.
         */
        SSLerr(SSL_F_DTLS1_PREPROCESS_FRAGMENT, SSL_R_EXCESSIVE_MESSAGE_SIZE);
        return SSL_AD_ILLEGAL_PARAMETER;
    }

    return 0;                   /* no error */
}