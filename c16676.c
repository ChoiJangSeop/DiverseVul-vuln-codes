static int asn1_d2i_read_bio(BIO *in, BUF_MEM **pb)
{
    BUF_MEM *b;
    unsigned char *p;
    int i;
    size_t want = HEADER_SIZE;
    int eos = 0;
    size_t off = 0;
    size_t len = 0;

    const unsigned char *q;
    long slen;
    int inf, tag, xclass;

    b = BUF_MEM_new();
    if (b == NULL) {
        ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ERR_R_MALLOC_FAILURE);
        return -1;
    }

    ERR_clear_error();
    for (;;) {
        if (want >= (len - off)) {
            want -= (len - off);

            if (len + want < len || !BUF_MEM_grow_clean(b, len + want)) {
                ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ERR_R_MALLOC_FAILURE);
                goto err;
            }
            i = BIO_read(in, &(b->data[len]), want);
            if ((i < 0) && ((len - off) == 0)) {
                ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_NOT_ENOUGH_DATA);
                goto err;
            }
            if (i > 0) {
                if (len + i < len) {
                    ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_TOO_LONG);
                    goto err;
                }
                len += i;
            }
        }
        /* else data already loaded */

        p = (unsigned char *)&(b->data[off]);
        q = p;
        inf = ASN1_get_object(&q, &slen, &tag, &xclass, len - off);
        if (inf & 0x80) {
            unsigned long e;

            e = ERR_GET_REASON(ERR_peek_error());
            if (e != ASN1_R_TOO_LONG)
                goto err;
            else
                ERR_clear_error(); /* clear error */
        }
        i = q - p;            /* header length */
        off += i;               /* end of data */

        if (inf & 1) {
            /* no data body so go round again */
            eos++;
            if (eos < 0) {
                ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_HEADER_TOO_LONG);
                goto err;
            }
            want = HEADER_SIZE;
        } else if (eos && (slen == 0) && (tag == V_ASN1_EOC)) {
            /* eos value, so go back and read another header */
            eos--;
            if (eos <= 0)
                break;
            else
                want = HEADER_SIZE;
        } else {
            /* suck in slen bytes of data */
            want = slen;
            if (want > (len - off)) {
                want -= (len - off);
                if (want > INT_MAX /* BIO_read takes an int length */  ||
                    len + want < len) {
                    ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_TOO_LONG);
                    goto err;
                }
                if (!BUF_MEM_grow_clean(b, len + want)) {
                    ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ERR_R_MALLOC_FAILURE);
                    goto err;
                }
                while (want > 0) {
                    i = BIO_read(in, &(b->data[len]), want);
                    if (i <= 0) {
                        ASN1err(ASN1_F_ASN1_D2I_READ_BIO,
                                ASN1_R_NOT_ENOUGH_DATA);
                        goto err;
                    }
                    /*
                     * This can't overflow because |len+want| didn't
                     * overflow.
                     */
                    len += i;
                    want -= i;
                }
            }
            if (off + slen < off) {
                ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_TOO_LONG);
                goto err;
            }
            off += slen;
            if (eos <= 0) {
                break;
            } else
                want = HEADER_SIZE;
        }
    }

    if (off > INT_MAX) {
        ASN1err(ASN1_F_ASN1_D2I_READ_BIO, ASN1_R_TOO_LONG);
        goto err;
    }

    *pb = b;
    return off;
 err:
    BUF_MEM_free(b);
    return -1;
}