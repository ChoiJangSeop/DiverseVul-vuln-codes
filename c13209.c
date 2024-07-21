void pointZZ_pMul(PointZZ_p * rop, const PointZZ_p * point, const mpz_t scalar, const CurveZZ_p * curve) {
    PointZZ_p R0, R1, tmp;
    mpz_inits(R1.x, R1.y, tmp.x, tmp.y, NULL);
    mpz_init_set(R0.x, point->x);
    mpz_init_set(R0.y, point->y);
    pointZZ_pDouble(&R1, point, curve);

    int dbits = mpz_sizeinbase(scalar, 2), i;

    for(i = dbits - 2; i >= 0; i--) {
        if(mpz_tstbit(scalar, i)) {
            mpz_set(tmp.x, R0.x);
            mpz_set(tmp.y, R0.y);
            pointZZ_pAdd(&R0, &R1, &tmp, curve);
            mpz_set(tmp.x, R1.x);
            mpz_set(tmp.y, R1.y);
            pointZZ_pDouble(&R1, &tmp, curve);
        }
        else {
            mpz_set(tmp.x, R1.x);
            mpz_set(tmp.y, R1.y);
            pointZZ_pAdd(&R1, &R0, &tmp, curve);
            mpz_set(tmp.x, R0.x);
            mpz_set(tmp.y, R0.y);
            pointZZ_pDouble(&R0, &tmp, curve);
        }
    }

    mpz_init_set(rop->x, R0.x);
    mpz_init_set(rop->y, R0.y);
    mpz_clears(R0.x, R0.y, R1.x, R1.y, tmp.x, tmp.y, NULL);
}