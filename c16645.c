static int trust_1oid(X509_TRUST *trust, X509 *x, int flags)
{
    if (x->aux && (x->aux->trust || x->aux->reject))
        return obj_trust(trust->arg1, x, flags);
    return X509_TRUST_UNTRUSTED;
}