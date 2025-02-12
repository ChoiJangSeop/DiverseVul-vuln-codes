int tls1_enc(SSL *s, int send)
{
    SSL3_RECORD *rec;
    EVP_CIPHER_CTX *ds;
    unsigned long l;
    int bs, i, j, k, pad = 0, ret, mac_size = 0;
    const EVP_CIPHER *enc;

    if (send) {
        if (EVP_MD_CTX_md(s->write_hash)) {
            int n = EVP_MD_CTX_size(s->write_hash);
            OPENSSL_assert(n >= 0);
        }
        ds = s->enc_write_ctx;
        rec = &(s->s3->wrec);
        if (s->enc_write_ctx == NULL)
            enc = NULL;
        else {
            int ivlen;
            enc = EVP_CIPHER_CTX_cipher(s->enc_write_ctx);
            /* For TLSv1.1 and later explicit IV */
            if (SSL_USE_EXPLICIT_IV(s)
                && EVP_CIPHER_mode(enc) == EVP_CIPH_CBC_MODE)
                ivlen = EVP_CIPHER_iv_length(enc);
            else
                ivlen = 0;
            if (ivlen > 1) {
                if (rec->data != rec->input)
                    /*
                     * we can't write into the input stream: Can this ever
                     * happen?? (steve)
                     */
                    fprintf(stderr,
                            "%s:%d: rec->data != rec->input\n",
                            __FILE__, __LINE__);
                else if (RAND_bytes(rec->input, ivlen) <= 0)
                    return -1;
            }
        }
    } else {
        if (EVP_MD_CTX_md(s->read_hash)) {
            int n = EVP_MD_CTX_size(s->read_hash);
            OPENSSL_assert(n >= 0);
        }
        ds = s->enc_read_ctx;
        rec = &(s->s3->rrec);
        if (s->enc_read_ctx == NULL)
            enc = NULL;
        else
            enc = EVP_CIPHER_CTX_cipher(s->enc_read_ctx);
    }

#ifdef KSSL_DEBUG
    fprintf(stderr, "tls1_enc(%d)\n", send);
#endif                          /* KSSL_DEBUG */

    if ((s->session == NULL) || (ds == NULL) || (enc == NULL)) {
        memmove(rec->data, rec->input, rec->length);
        rec->input = rec->data;
        ret = 1;
    } else {
        l = rec->length;
        bs = EVP_CIPHER_block_size(ds->cipher);

        if (EVP_CIPHER_flags(ds->cipher) & EVP_CIPH_FLAG_AEAD_CIPHER) {
            unsigned char buf[13], *seq;

            seq = send ? s->s3->write_sequence : s->s3->read_sequence;

            if (SSL_IS_DTLS(s)) {
                unsigned char dtlsseq[9], *p = dtlsseq;

                s2n(send ? s->d1->w_epoch : s->d1->r_epoch, p);
                memcpy(p, &seq[2], 6);
                memcpy(buf, dtlsseq, 8);
            } else {
                memcpy(buf, seq, 8);
                for (i = 7; i >= 0; i--) { /* increment */
                    ++seq[i];
                    if (seq[i] != 0)
                        break;
                }
            }

            buf[8] = rec->type;
            buf[9] = (unsigned char)(s->version >> 8);
            buf[10] = (unsigned char)(s->version);
            buf[11] = rec->length >> 8;
            buf[12] = rec->length & 0xff;
            pad = EVP_CIPHER_CTX_ctrl(ds, EVP_CTRL_AEAD_TLS1_AAD, 13, buf);
            if (send) {
                l += pad;
                rec->length += pad;
            }
        } else if ((bs != 1) && send) {
            i = bs - ((int)l % bs);

            /* Add weird padding of upto 256 bytes */

            /* we need to add 'i' padding bytes of value j */
            j = i - 1;
            if (s->options & SSL_OP_TLS_BLOCK_PADDING_BUG) {
                if (s->s3->flags & TLS1_FLAGS_TLS_PADDING_BUG)
                    j++;
            }
            for (k = (int)l; k < (int)(l + i); k++)
                rec->input[k] = j;
            l += i;
            rec->length += i;
        }
#ifdef KSSL_DEBUG
        {
            unsigned long ui;
            fprintf(stderr,
                    "EVP_Cipher(ds=%p,rec->data=%p,rec->input=%p,l=%ld) ==>\n",
                    ds, rec->data, rec->input, l);
            fprintf(stderr,
                    "\tEVP_CIPHER_CTX: %d buf_len, %d key_len [%lu %lu], %d iv_len\n",
                    ds->buf_len, ds->cipher->key_len, DES_KEY_SZ,
                    DES_SCHEDULE_SZ, ds->cipher->iv_len);
            fprintf(stderr, "\t\tIV: ");
            for (i = 0; i < ds->cipher->iv_len; i++)
                fprintf(stderr, "%02X", ds->iv[i]);
            fprintf(stderr, "\n");
            fprintf(stderr, "\trec->input=");
            for (ui = 0; ui < l; ui++)
                fprintf(stderr, " %02x", rec->input[ui]);
            fprintf(stderr, "\n");
        }
#endif                          /* KSSL_DEBUG */

        if (!send) {
            if (l == 0 || l % bs != 0)
                return 0;
        }

        i = EVP_Cipher(ds, rec->data, rec->input, l);
        if ((EVP_CIPHER_flags(ds->cipher) & EVP_CIPH_FLAG_CUSTOM_CIPHER)
            ? (i < 0)
            : (i == 0))
            return -1;          /* AEAD can fail to verify MAC */
        if (EVP_CIPHER_mode(enc) == EVP_CIPH_GCM_MODE && !send) {
            rec->data += EVP_GCM_TLS_EXPLICIT_IV_LEN;
            rec->input += EVP_GCM_TLS_EXPLICIT_IV_LEN;
            rec->length -= EVP_GCM_TLS_EXPLICIT_IV_LEN;
        }
#ifdef KSSL_DEBUG
        {
            unsigned long i;
            fprintf(stderr, "\trec->data=");
            for (i = 0; i < l; i++)
                fprintf(stderr, " %02x", rec->data[i]);
            fprintf(stderr, "\n");
        }
#endif                          /* KSSL_DEBUG */

        ret = 1;
        if (EVP_MD_CTX_md(s->read_hash) != NULL)
            mac_size = EVP_MD_CTX_size(s->read_hash);
        if ((bs != 1) && !send)
            ret = tls1_cbc_remove_padding(s, rec, bs, mac_size);
        if (pad && !send)
            rec->length -= pad;
    }
    return ret;
}