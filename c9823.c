static OPJ_BOOL opj_pi_next_lrcp(opj_pi_iterator_t * pi)
{
    opj_pi_comp_t *comp = NULL;
    opj_pi_resolution_t *res = NULL;
    OPJ_UINT32 index = 0;

    if (!pi->first) {
        comp = &pi->comps[pi->compno];
        res = &comp->resolutions[pi->resno];
        goto LABEL_SKIP;
    } else {
        pi->first = 0;
    }

    for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
        for (pi->resno = pi->poc.resno0; pi->resno < pi->poc.resno1;
                pi->resno++) {
            for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
                comp = &pi->comps[pi->compno];
                if (pi->resno >= comp->numresolutions) {
                    continue;
                }
                res = &comp->resolutions[pi->resno];
                if (!pi->tp_on) {
                    pi->poc.precno1 = res->pw * res->ph;
                }
                for (pi->precno = pi->poc.precno0; pi->precno < pi->poc.precno1; pi->precno++) {
                    index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno *
                            pi->step_c + pi->precno * pi->step_p;
                    /* Avoids index out of bounds access with */
                    /* id_000098,sig_11,src_005411,op_havoc,rep_2 of */
                    /* https://github.com/uclouvain/openjpeg/issues/938 */
                    /* Not sure if this is the most clever fix. Perhaps */
                    /* include should be resized when a POC arises, or */
                    /* the POC should be rejected */
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

    return OPJ_FALSE;
}