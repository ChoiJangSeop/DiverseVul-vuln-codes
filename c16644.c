int X509_check_trust(X509 *x, int id, int flags)
{
    X509_TRUST *pt;
    int idx;

    /* We get this as a default value */
    if (id == 0) {
        int rv;
        rv = obj_trust(NID_anyExtendedKeyUsage, x, 0);
        if (rv != X509_TRUST_UNTRUSTED)
            return rv;
        return trust_compat(NULL, x, 0);
    }
    idx = X509_TRUST_get_by_id(id);
    if (idx == -1)
        return default_trust(id, x, flags);
    pt = X509_TRUST_get0(idx);
    return pt->check_trust(pt, x, flags);
}