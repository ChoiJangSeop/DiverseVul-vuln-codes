NOEXPORT char *base64(int encode, const char *in, int len) {
    BIO *bio, *b64;
    char *out;
    int n;

    b64=BIO_new(BIO_f_base64());
    if(!b64)
        return NULL;
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio=BIO_new(BIO_s_mem());
    if(!bio) {
        str_free(b64);
        return NULL;
    }
    if(encode)
        bio=BIO_push(b64, bio);
    BIO_write(bio, in, len);
    (void)BIO_flush(bio); /* ignore the error if any */
    if(encode) {
        bio=BIO_pop(bio);
        BIO_free(b64);
    } else {
        bio=BIO_push(b64, bio);
    }
    n=BIO_pending(bio);
    /* 32 bytes as a safety precaution for passing decoded data to crypt_DES */
    /* n+1 to get null-terminated string on encode */
    out=str_alloc(n<32?32:(size_t)n+1);
    n=BIO_read(bio, out, n);
    if(n<0) {
        BIO_free_all(bio);
        str_free(out);
        return NULL;
    }
    BIO_free_all(bio);
    return out;
}