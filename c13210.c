void pointZZ_pAdd(PointZZ_p * rop, const PointZZ_p * op1, const PointZZ_p * op2, const CurveZZ_p * curve) {
    mpz_t xdiff, ydiff, lambda;
    mpz_inits(xdiff, ydiff, lambda, NULL);

    // calculate lambda
    mpz_sub(ydiff, op2->y, op1->y);
    mpz_sub(xdiff, op2->x, op1->x);
    mpz_invert(xdiff, xdiff, curve->p);  // TODO check status
    mpz_mul(lambda, ydiff, xdiff);
    mpz_mod(lambda, lambda, curve->p);

    // calculate resulting x coord
    mpz_mul(rop->x, lambda, lambda);
    mpz_sub(rop->x, rop->x, op1->x);
    mpz_sub(rop->x, rop->x, op2->x);
    mpz_mod(rop->x, rop->x, curve->p);

    //calculate resulting y coord
    mpz_sub(rop->y, op1->x, rop->x);
    mpz_mul(rop->y, lambda, rop->y);
    mpz_sub(rop->y, rop->y, op1->y);
    mpz_mod(rop->y, rop->y, curve->p);

    mpz_clears(xdiff, ydiff, lambda, NULL);
}