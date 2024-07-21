X509 *d2i_X509_AUX(X509 **a, const unsigned char **pp, long length)
{
    const unsigned char *q;
    X509 *ret;
    /* Save start position */
    q = *pp;
    ret = d2i_X509(a, pp, length);
    /* If certificate unreadable then forget it */
    if (!ret)
        return NULL;
    /* update length */
    length -= *pp - q;
    if (!length)
        return ret;
    if (!d2i_X509_CERT_AUX(&ret->aux, pp, length))
        goto err;
    return ret;
 err:
    X509_free(ret);
    return NULL;
}