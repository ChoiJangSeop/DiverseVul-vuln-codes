X509_NAME_oneline_ex(X509_NAME * a,
                     char *buf,
                     unsigned int *size,
                     unsigned long flag)
{
    BIO *out = NULL;

    out = BIO_new(BIO_s_mem ());
    if (X509_NAME_print_ex(out, a, 0, flag) > 0) {
        if (buf != NULL && (*size) >  (unsigned int) BIO_number_written(out)) {
            memset(buf, 0, *size);
            BIO_read(out, buf, (int) BIO_number_written(out));
        }
        else {
            *size = BIO_number_written(out);
        }
    }
    BIO_free(out);
    return (buf);
}