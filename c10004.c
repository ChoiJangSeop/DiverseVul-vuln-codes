int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g) {
    if(!p || !g) /* q is optional */
        return 0;
    BN_free(dh->p);
    BN_free(dh->q);
    BN_free(dh->g);
    dh->p = p;
    dh->q = q;
    dh->g = g;
    if(q)
        dh->length = BN_num_bits(q);
    return 1;
}