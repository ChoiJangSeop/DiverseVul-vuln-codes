int ssl3_write_bytes(SSL *s, int type, const void *buf_, int len)
{
    const unsigned char *buf = buf_;
    int tot;
    unsigned int n, nw;
#if !defined(OPENSSL_NO_MULTIBLOCK) && EVP_CIPH_FLAG_TLS1_1_MULTIBLOCK
    unsigned int max_send_fragment;
#endif
    SSL3_BUFFER *wb = &(s->s3->wbuf);
    int i;

    s->rwstate = SSL_NOTHING;
    OPENSSL_assert(s->s3->wnum <= INT_MAX);
    tot = s->s3->wnum;
    s->s3->wnum = 0;

    if (SSL_in_init(s) && !s->in_handshake) {
        i = s->handshake_func(s);
        if (i < 0)
            return (i);
        if (i == 0) {
            SSLerr(SSL_F_SSL3_WRITE_BYTES, SSL_R_SSL_HANDSHAKE_FAILURE);
            return -1;
        }
    }

    /*
     * ensure that if we end up with a smaller value of data to write out
     * than the the original len from a write which didn't complete for
     * non-blocking I/O and also somehow ended up avoiding the check for
     * this in ssl3_write_pending/SSL_R_BAD_WRITE_RETRY as it must never be
     * possible to end up with (len-tot) as a large number that will then
     * promptly send beyond the end of the users buffer ... so we trap and
     * report the error in a way the user will notice
     */
    if (len < tot) {
        SSLerr(SSL_F_SSL3_WRITE_BYTES, SSL_R_BAD_LENGTH);
        return (-1);
    }

    /*
     * first check if there is a SSL3_BUFFER still being written out.  This
     * will happen with non blocking IO
     */
    if (wb->left != 0) {
        i = ssl3_write_pending(s, type, &buf[tot], s->s3->wpend_tot);
        if (i <= 0) {
            /* XXX should we ssl3_release_write_buffer if i<0? */
            s->s3->wnum = tot;
            return i;
        }
        tot += i;               /* this might be last fragment */
    }
#if !defined(OPENSSL_NO_MULTIBLOCK) && EVP_CIPH_FLAG_TLS1_1_MULTIBLOCK
    /*
     * Depending on platform multi-block can deliver several *times*
     * better performance. Downside is that it has to allocate
     * jumbo buffer to accomodate up to 8 records, but the
     * compromise is considered worthy.
     */
    if (type == SSL3_RT_APPLICATION_DATA &&
        len >= 4 * (int)(max_send_fragment = s->max_send_fragment) &&
        s->compress == NULL && s->msg_callback == NULL &&
        SSL_USE_EXPLICIT_IV(s) &&
        EVP_CIPHER_flags(s->enc_write_ctx->cipher) &
        EVP_CIPH_FLAG_TLS1_1_MULTIBLOCK) {
        unsigned char aad[13];
        EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM mb_param;
        int packlen;

        /* minimize address aliasing conflicts */
        if ((max_send_fragment & 0xfff) == 0)
            max_send_fragment -= 512;

        if (tot == 0 || wb->buf == NULL) { /* allocate jumbo buffer */
            ssl3_release_write_buffer(s);

            packlen = EVP_CIPHER_CTX_ctrl(s->enc_write_ctx,
                                          EVP_CTRL_TLS1_1_MULTIBLOCK_MAX_BUFSIZE,
                                          max_send_fragment, NULL);

            if (len >= 8 * (int)max_send_fragment)
                packlen *= 8;
            else
                packlen *= 4;

            wb->buf = OPENSSL_malloc(packlen);
            if(!wb->buf) {
                SSLerr(SSL_F_SSL3_WRITE_BYTES, ERR_R_MALLOC_FAILURE);
                return -1;
            }
            wb->len = packlen;
        } else if (tot == len) { /* done? */
            OPENSSL_free(wb->buf); /* free jumbo buffer */
            wb->buf = NULL;
            return tot;
        }

        n = (len - tot);
        for (;;) {
            if (n < 4 * max_send_fragment) {
                OPENSSL_free(wb->buf); /* free jumbo buffer */
                wb->buf = NULL;
                break;
            }

            if (s->s3->alert_dispatch) {
                i = s->method->ssl_dispatch_alert(s);
                if (i <= 0) {
                    s->s3->wnum = tot;
                    return i;
                }
            }

            if (n >= 8 * max_send_fragment)
                nw = max_send_fragment * (mb_param.interleave = 8);
            else
                nw = max_send_fragment * (mb_param.interleave = 4);

            memcpy(aad, s->s3->write_sequence, 8);
            aad[8] = type;
            aad[9] = (unsigned char)(s->version >> 8);
            aad[10] = (unsigned char)(s->version);
            aad[11] = 0;
            aad[12] = 0;
            mb_param.out = NULL;
            mb_param.inp = aad;
            mb_param.len = nw;

            packlen = EVP_CIPHER_CTX_ctrl(s->enc_write_ctx,
                                          EVP_CTRL_TLS1_1_MULTIBLOCK_AAD,
                                          sizeof(mb_param), &mb_param);

            if (packlen <= 0 || packlen > (int)wb->len) { /* never happens */
                OPENSSL_free(wb->buf); /* free jumbo buffer */
                wb->buf = NULL;
                break;
            }

            mb_param.out = wb->buf;
            mb_param.inp = &buf[tot];
            mb_param.len = nw;

            if (EVP_CIPHER_CTX_ctrl(s->enc_write_ctx,
                                    EVP_CTRL_TLS1_1_MULTIBLOCK_ENCRYPT,
                                    sizeof(mb_param), &mb_param) <= 0)
                return -1;

            s->s3->write_sequence[7] += mb_param.interleave;
            if (s->s3->write_sequence[7] < mb_param.interleave) {
                int j = 6;
                while (j >= 0 && (++s->s3->write_sequence[j--]) == 0) ;
            }

            wb->offset = 0;
            wb->left = packlen;

            s->s3->wpend_tot = nw;
            s->s3->wpend_buf = &buf[tot];
            s->s3->wpend_type = type;
            s->s3->wpend_ret = nw;

            i = ssl3_write_pending(s, type, &buf[tot], nw);
            if (i <= 0) {
                if (i < 0) {
                    OPENSSL_free(wb->buf);
                    wb->buf = NULL;
                }
                s->s3->wnum = tot;
                return i;
            }
            if (i == (int)n) {
                OPENSSL_free(wb->buf); /* free jumbo buffer */
                wb->buf = NULL;
                return tot + i;
            }
            n -= i;
            tot += i;
        }
    } else
#endif
    if (tot == len) {           /* done? */
        if (s->mode & SSL_MODE_RELEASE_BUFFERS && !SSL_IS_DTLS(s))
            ssl3_release_write_buffer(s);

        return tot;
    }

    n = (len - tot);
    for (;;) {
        if (n > s->max_send_fragment)
            nw = s->max_send_fragment;
        else
            nw = n;

        i = do_ssl3_write(s, type, &(buf[tot]), nw, 0);
        if (i <= 0) {
            /* XXX should we ssl3_release_write_buffer if i<0? */
            s->s3->wnum = tot;
            return i;
        }

        if ((i == (int)n) ||
            (type == SSL3_RT_APPLICATION_DATA &&
             (s->mode & SSL_MODE_ENABLE_PARTIAL_WRITE))) {
            /*
             * next chunk of data should get another prepended empty fragment
             * in ciphersuites with known-IV weakness:
             */
            s->s3->empty_fragment_done = 0;

            if ((i == (int)n) && s->mode & SSL_MODE_RELEASE_BUFFERS &&
                !SSL_IS_DTLS(s))
                ssl3_release_write_buffer(s);

            return tot + i;
        }

        n -= i;
        tot += i;
    }
}