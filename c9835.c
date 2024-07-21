OPJ_BOOL opj_tcd_rateallocate(opj_tcd_t *tcd,
                              OPJ_BYTE *dest,
                              OPJ_UINT32 * p_data_written,
                              OPJ_UINT32 len,
                              opj_codestream_info_t *cstr_info,
                              opj_event_mgr_t *p_manager)
{
    OPJ_UINT32 compno, resno, bandno, precno, cblkno, layno;
    OPJ_UINT32 passno;
    OPJ_FLOAT64 min, max;
    OPJ_FLOAT64 cumdisto[100];      /* fixed_quality */
    const OPJ_FLOAT64 K = 1;                /* 1.1; fixed_quality */
    OPJ_FLOAT64 maxSE = 0;

    opj_cp_t *cp = tcd->cp;
    opj_tcd_tile_t *tcd_tile = tcd->tcd_image->tiles;
    opj_tcp_t *tcd_tcp = tcd->tcp;

    min = DBL_MAX;
    max = 0;

    tcd_tile->numpix = 0;           /* fixed_quality */

    for (compno = 0; compno < tcd_tile->numcomps; compno++) {
        opj_tcd_tilecomp_t *tilec = &tcd_tile->comps[compno];
        tilec->numpix = 0;

        for (resno = 0; resno < tilec->numresolutions; resno++) {
            opj_tcd_resolution_t *res = &tilec->resolutions[resno];

            for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_band_t *band = &res->bands[bandno];

                /* Skip empty bands */
                if (opj_tcd_is_band_empty(band)) {
                    continue;
                }

                for (precno = 0; precno < res->pw * res->ph; precno++) {
                    opj_tcd_precinct_t *prc = &band->precincts[precno];

                    for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        opj_tcd_cblk_enc_t *cblk = &prc->cblks.enc[cblkno];

                        for (passno = 0; passno < cblk->totalpasses; passno++) {
                            opj_tcd_pass_t *pass = &cblk->passes[passno];
                            OPJ_INT32 dr;
                            OPJ_FLOAT64 dd, rdslope;

                            if (passno == 0) {
                                dr = (OPJ_INT32)pass->rate;
                                dd = pass->distortiondec;
                            } else {
                                dr = (OPJ_INT32)(pass->rate - cblk->passes[passno - 1].rate);
                                dd = pass->distortiondec - cblk->passes[passno - 1].distortiondec;
                            }

                            if (dr == 0) {
                                continue;
                            }

                            rdslope = dd / dr;
                            if (rdslope < min) {
                                min = rdslope;
                            }

                            if (rdslope > max) {
                                max = rdslope;
                            }
                        } /* passno */

                        /* fixed_quality */
                        tcd_tile->numpix += ((cblk->x1 - cblk->x0) * (cblk->y1 - cblk->y0));
                        tilec->numpix += ((cblk->x1 - cblk->x0) * (cblk->y1 - cblk->y0));
                    } /* cbklno */
                } /* precno */
            } /* bandno */
        } /* resno */

        maxSE += (((OPJ_FLOAT64)(1 << tcd->image->comps[compno].prec) - 1.0)
                  * ((OPJ_FLOAT64)(1 << tcd->image->comps[compno].prec) - 1.0))
                 * ((OPJ_FLOAT64)(tilec->numpix));
    } /* compno */

    /* index file */
    if (cstr_info) {
        opj_tile_info_t *tile_info = &cstr_info->tile[tcd->tcd_tileno];
        tile_info->numpix = tcd_tile->numpix;
        tile_info->distotile = tcd_tile->distotile;
        tile_info->thresh = (OPJ_FLOAT64 *) opj_malloc(tcd_tcp->numlayers * sizeof(
                                OPJ_FLOAT64));
        if (!tile_info->thresh) {
            /* FIXME event manager error callback */
            return OPJ_FALSE;
        }
    }

    for (layno = 0; layno < tcd_tcp->numlayers; layno++) {
        OPJ_FLOAT64 lo = min;
        OPJ_FLOAT64 hi = max;
        OPJ_UINT32 maxlen = tcd_tcp->rates[layno] > 0.0f ? opj_uint_min(((
                                OPJ_UINT32) ceil(tcd_tcp->rates[layno])), len) : len;
        OPJ_FLOAT64 goodthresh = 0;
        OPJ_FLOAT64 stable_thresh = 0;
        OPJ_UINT32 i;
        OPJ_FLOAT64 distotarget;                /* fixed_quality */

        /* fixed_quality */
        distotarget = tcd_tile->distotile - ((K * maxSE) / pow((OPJ_FLOAT32)10,
                                             tcd_tcp->distoratio[layno] / 10));

        /* Don't try to find an optimal threshold but rather take everything not included yet, if
          -r xx,yy,zz,0   (disto_alloc == 1 and rates == 0)
          -q xx,yy,zz,0   (fixed_quality == 1 and distoratio == 0)
          ==> possible to have some lossy layers and the last layer for sure lossless */
        if (((cp->m_specific_param.m_enc.m_disto_alloc == 1) &&
                (tcd_tcp->rates[layno] > 0.0f)) ||
                ((cp->m_specific_param.m_enc.m_fixed_quality == 1) &&
                 (tcd_tcp->distoratio[layno] > 0.0))) {
            opj_t2_t*t2 = opj_t2_create(tcd->image, cp);
            OPJ_FLOAT64 thresh = 0;

            if (t2 == 00) {
                return OPJ_FALSE;
            }

            for (i = 0; i < 128; ++i) {
                OPJ_FLOAT64 distoachieved = 0;  /* fixed_quality */

                thresh = (lo + hi) / 2;

                opj_tcd_makelayer(tcd, layno, thresh, 0);

                if (cp->m_specific_param.m_enc.m_fixed_quality) {       /* fixed_quality */
                    if (OPJ_IS_CINEMA(cp->rsiz) || OPJ_IS_IMF(cp->rsiz)) {
                        if (! opj_t2_encode_packets(t2, tcd->tcd_tileno, tcd_tile, layno + 1, dest,
                                                    p_data_written, maxlen, cstr_info, tcd->cur_tp_num, tcd->tp_pos, tcd->cur_pino,
                                                    THRESH_CALC, p_manager)) {

                            lo = thresh;
                            continue;
                        } else {
                            distoachieved = layno == 0 ?
                                            tcd_tile->distolayer[0] : cumdisto[layno - 1] + tcd_tile->distolayer[layno];

                            if (distoachieved < distotarget) {
                                hi = thresh;
                                stable_thresh = thresh;
                                continue;
                            } else {
                                lo = thresh;
                            }
                        }
                    } else {
                        distoachieved = (layno == 0) ?
                                        tcd_tile->distolayer[0] : (cumdisto[layno - 1] + tcd_tile->distolayer[layno]);

                        if (distoachieved < distotarget) {
                            hi = thresh;
                            stable_thresh = thresh;
                            continue;
                        }
                        lo = thresh;
                    }
                } else {
                    if (! opj_t2_encode_packets(t2, tcd->tcd_tileno, tcd_tile, layno + 1, dest,
                                                p_data_written, maxlen, cstr_info, tcd->cur_tp_num, tcd->tp_pos, tcd->cur_pino,
                                                THRESH_CALC, p_manager)) {
                        /* TODO: what to do with l ??? seek / tell ??? */
                        /* opj_event_msg(tcd->cinfo, EVT_INFO, "rate alloc: len=%d, max=%d\n", l, maxlen); */
                        lo = thresh;
                        continue;
                    }

                    hi = thresh;
                    stable_thresh = thresh;
                }
            }

            goodthresh = stable_thresh == 0 ? thresh : stable_thresh;

            opj_t2_destroy(t2);
        } else {
            /* Special value to indicate to use all passes */
            goodthresh = -1;
        }

        if (cstr_info) { /* Threshold for Marcela Index */
            cstr_info->tile[tcd->tcd_tileno].thresh[layno] = goodthresh;
        }

        opj_tcd_makelayer(tcd, layno, goodthresh, 1);

        /* fixed_quality */
        cumdisto[layno] = (layno == 0) ? tcd_tile->distolayer[0] :
                          (cumdisto[layno - 1] + tcd_tile->distolayer[layno]);
    }

    return OPJ_TRUE;
}