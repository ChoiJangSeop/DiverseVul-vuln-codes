static int obj_trust(int id, X509 *x, int flags)
{
    ASN1_OBJECT *obj;
    int i;
    X509_CERT_AUX *ax;
    ax = x->aux;
    if (!ax)
        return X509_TRUST_UNTRUSTED;
    if (ax->reject) {
        for (i = 0; i < sk_ASN1_OBJECT_num(ax->reject); i++) {
            obj = sk_ASN1_OBJECT_value(ax->reject, i);
            if (OBJ_obj2nid(obj) == id)
                return X509_TRUST_REJECTED;
        }
    }
    if (ax->trust) {
        for (i = 0; i < sk_ASN1_OBJECT_num(ax->trust); i++) {
            obj = sk_ASN1_OBJECT_value(ax->trust, i);
            if (OBJ_obj2nid(obj) == id)
                return X509_TRUST_TRUSTED;
        }
    }
    return X509_TRUST_UNTRUSTED;
}