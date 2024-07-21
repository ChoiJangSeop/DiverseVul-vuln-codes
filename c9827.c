static OPJ_BOOL opj_pi_next_pcrl(opj_pi_iterator_t * pi)
{
    opj_pi_comp_t *comp = NULL;
    opj_pi_resolution_t *res = NULL;
    OPJ_UINT32 index = 0;

    if (!pi->first) {
        comp = &pi->comps[pi->compno];
        goto LABEL_SKIP;
    } else {
        OPJ_UINT32 compno, resno;
        pi->first = 0;
        pi->dx = 0;
        pi->dy = 0;
        for (compno = 0; compno < pi->numcomps; compno++) {
            comp = &pi->comps[compno];
            for (resno = 0; resno < comp->numresolutions; resno++) {
                OPJ_UINT32 dx, dy;
                res = &comp->resolutions[resno];
                if (res->pdx + comp->numresolutions - 1 - resno < 32 &&
                        comp->dx <= UINT_MAX / (1u << (res->pdx + comp->numresolutions - 1 - resno))) {
                    dx = comp->dx * (1u << (res->pdx + comp->numresolutions - 1 - resno));
                    pi->dx = !pi->dx ? dx : opj_uint_min(pi->dx, dx);
                }
                if (res->pdy + comp->numresolutions - 1 - resno < 32 &&
                        comp->dy <= UINT_MAX / (1u << (res->pdy + comp->numresolutions - 1 - resno))) {
                    dy = comp->dy * (1u << (res->pdy + comp->numresolutions - 1 - resno));
                    pi->dy = !pi->dy ? dy : opj_uint_min(pi->dy, dy);
                }
            }
        }
        if (pi->dx == 0 || pi->dy == 0) {
            return OPJ_FALSE;
        }
    }
    if (!pi->tp_on) {
        pi->poc.ty0 = pi->ty0;
        pi->poc.tx0 = pi->tx0;
        pi->poc.ty1 = pi->ty1;
        pi->poc.tx1 = pi->tx1;
    }
    for (pi->y = (OPJ_UINT32)pi->poc.ty0; pi->y < (OPJ_UINT32)pi->poc.ty1;
            pi->y += (pi->dy - (pi->y % pi->dy))) {
        for (pi->x = (OPJ_UINT32)pi->poc.tx0; pi->x < (OPJ_UINT32)pi->poc.tx1;
                pi->x += (pi->dx - (pi->x % pi->dx))) {
            for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
                comp = &pi->comps[pi->compno];
                for (pi->resno = pi->poc.resno0;
                        pi->resno < opj_uint_min(pi->poc.resno1, comp->numresolutions); pi->resno++) {
                    OPJ_UINT32 levelno;
                    OPJ_UINT32 trx0, try0;
                    OPJ_UINT32 trx1, try1;
                    OPJ_UINT32 rpx, rpy;
                    OPJ_UINT32 prci, prcj;
                    res = &comp->resolutions[pi->resno];
                    levelno = comp->numresolutions - 1 - pi->resno;
                    /* Avoids division by zero */
                    /* Relates to id_000004,sig_06,src_000679,op_arith8,pos_49,val_-17 */
                    /* of  https://github.com/uclouvain/openjpeg/issues/938 */
                    if (levelno >= 32 ||
                            ((comp->dx << levelno) >> levelno) != comp->dx ||
                            ((comp->dy << levelno) >> levelno) != comp->dy) {
                        continue;
                    }
                    if ((comp->dx << levelno) > INT_MAX ||
                            (comp->dy << levelno) > INT_MAX) {
                        continue;
                    }
                    trx0 = opj_uint_ceildiv(pi->tx0, (comp->dx << levelno));
                    try0 = opj_uint_ceildiv(pi->ty0, (comp->dy << levelno));
                    trx1 = opj_uint_ceildiv(pi->tx1, (comp->dx << levelno));
                    try1 = opj_uint_ceildiv(pi->ty1, (comp->dy << levelno));
                    rpx = res->pdx + levelno;
                    rpy = res->pdy + levelno;

                    /* To avoid divisions by zero / undefined behaviour on shift */
                    /* in below tests */
                    /* Relates to id:000019,sig:08,src:001098,op:flip1,pos:49 */
                    /* of https://github.com/uclouvain/openjpeg/issues/938 */
                    if (rpx >= 31 || ((comp->dx << rpx) >> rpx) != comp->dx ||
                            rpy >= 31 || ((comp->dy << rpy) >> rpy) != comp->dy) {
                        continue;
                    }

                    /* See ISO-15441. B.12.1.4 Position-component-resolution level-layer progression */
                    if (!((pi->y % (comp->dy << rpy) == 0) || ((pi->y == pi->ty0) &&
                            ((try0 << levelno) % (1U << rpy))))) {
                        continue;
                    }
                    if (!((pi->x % (comp->dx << rpx) == 0) || ((pi->x == pi->tx0) &&
                            ((trx0 << levelno) % (1U << rpx))))) {
                        continue;
                    }

                    if ((res->pw == 0) || (res->ph == 0)) {
                        continue;
                    }

                    if ((trx0 == trx1) || (try0 == try1)) {
                        continue;
                    }

                    prci = opj_uint_floordivpow2(opj_uint_ceildiv(pi->x,
                                                 (comp->dx << levelno)), res->pdx)
                           - opj_uint_floordivpow2(trx0, res->pdx);
                    prcj = opj_uint_floordivpow2(opj_uint_ceildiv(pi->y,
                                                 (comp->dy << levelno)), res->pdy)
                           - opj_uint_floordivpow2(try0, res->pdy);
                    pi->precno = prci + prcj * res->pw;
                    for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
                        index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno *
                                pi->step_c + pi->precno * pi->step_p;
                        if (index >= pi->include_size) {
                            opj_pi_emit_error(pi, "Invalid access to pi->include");
                            return OPJ_FALSE;
                        }
                        if (!pi->include[index]) {
                            pi->include[index] = 1;
                            return OPJ_TRUE;
                        }
LABEL_SKIP:
                        ;
                    }
                }
            }
        }
    }

    return OPJ_FALSE;
}