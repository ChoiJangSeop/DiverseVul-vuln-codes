OPJ_BOOL opj_t2_decode_packets(opj_tcd_t* tcd,
                               opj_t2_t *p_t2,
                               OPJ_UINT32 p_tile_no,
                               opj_tcd_tile_t *p_tile,
                               OPJ_BYTE *p_src,
                               OPJ_UINT32 * p_data_read,
                               OPJ_UINT32 p_max_len,
                               opj_codestream_index_t *p_cstr_index,
                               opj_event_mgr_t *p_manager)
{
    OPJ_BYTE *l_current_data = p_src;
    opj_pi_iterator_t *l_pi = 00;
    OPJ_UINT32 pino;
    opj_image_t *l_image = p_t2->image;
    opj_cp_t *l_cp = p_t2->cp;
    opj_tcp_t *l_tcp = &(p_t2->cp->tcps[p_tile_no]);
    OPJ_UINT32 l_nb_bytes_read;
    OPJ_UINT32 l_nb_pocs = l_tcp->numpocs + 1;
    opj_pi_iterator_t *l_current_pi = 00;
#ifdef TODO_MSD
    OPJ_UINT32 curtp = 0;
    OPJ_UINT32 tp_start_packno;
#endif
    opj_packet_info_t *l_pack_info = 00;
    opj_image_comp_t* l_img_comp = 00;

    OPJ_ARG_NOT_USED(p_cstr_index);

#ifdef TODO_MSD
    if (p_cstr_index) {
        l_pack_info = p_cstr_index->tile_index[p_tile_no].packet;
    }
#endif

    /* create a packet iterator */
    l_pi = opj_pi_create_decode(l_image, l_cp, p_tile_no);
    if (!l_pi) {
        return OPJ_FALSE;
    }


    l_current_pi = l_pi;

    for (pino = 0; pino <= l_tcp->numpocs; ++pino) {

        /* if the resolution needed is too low, one dim of the tilec could be equal to zero
         * and no packets are used to decode this resolution and
         * l_current_pi->resno is always >= p_tile->comps[l_current_pi->compno].minimum_num_resolutions
         * and no l_img_comp->resno_decoded are computed
         */
        OPJ_BOOL* first_pass_failed = NULL;

        if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
            /* TODO ADE : add an error */
            opj_pi_destroy(l_pi, l_nb_pocs);
            return OPJ_FALSE;
        }

        first_pass_failed = (OPJ_BOOL*)opj_malloc(l_image->numcomps * sizeof(OPJ_BOOL));
        if (!first_pass_failed) {
            opj_pi_destroy(l_pi, l_nb_pocs);
            return OPJ_FALSE;
        }
        memset(first_pass_failed, OPJ_TRUE, l_image->numcomps * sizeof(OPJ_BOOL));

        while (opj_pi_next(l_current_pi)) {
            OPJ_BOOL skip_packet = OPJ_FALSE;
            JAS_FPRINTF(stderr,
                        "packet offset=00000166 prg=%d cmptno=%02d rlvlno=%02d prcno=%03d lyrno=%02d\n\n",
                        l_current_pi->poc.prg1, l_current_pi->compno, l_current_pi->resno,
                        l_current_pi->precno, l_current_pi->layno);

            /* If the packet layer is greater or equal than the maximum */
            /* number of layers, skip the packet */
            if (l_current_pi->layno >= l_tcp->num_layers_to_decode) {
                skip_packet = OPJ_TRUE;
            }
            /* If the packet resolution number is greater than the minimum */
            /* number of resolution allowed, skip the packet */
            else if (l_current_pi->resno >=
                     p_tile->comps[l_current_pi->compno].minimum_num_resolutions) {
                skip_packet = OPJ_TRUE;
            } else {
                /* If no precincts of any band intersects the area of interest, */
                /* skip the packet */
                OPJ_UINT32 bandno;
                opj_tcd_tilecomp_t *tilec = &p_tile->comps[l_current_pi->compno];
                opj_tcd_resolution_t *res = &tilec->resolutions[l_current_pi->resno];

                skip_packet = OPJ_TRUE;
                for (bandno = 0; bandno < res->numbands; ++bandno) {
                    opj_tcd_band_t* band = &res->bands[bandno];
                    opj_tcd_precinct_t* prec = &band->precincts[l_current_pi->precno];

                    if (opj_tcd_is_subband_area_of_interest(tcd,
                                                            l_current_pi->compno,
                                                            l_current_pi->resno,
                                                            band->bandno,
                                                            (OPJ_UINT32)prec->x0,
                                                            (OPJ_UINT32)prec->y0,
                                                            (OPJ_UINT32)prec->x1,
                                                            (OPJ_UINT32)prec->y1)) {
                        skip_packet = OPJ_FALSE;
                        break;
                    }
                }
                /*
                                printf("packet cmptno=%02d rlvlno=%02d prcno=%03d lyrno=%02d -> %s\n",
                                    l_current_pi->compno, l_current_pi->resno,
                                    l_current_pi->precno, l_current_pi->layno, skip_packet ? "skipped" : "kept");
                */
            }

            if (!skip_packet) {
                l_nb_bytes_read = 0;

                first_pass_failed[l_current_pi->compno] = OPJ_FALSE;

                if (! opj_t2_decode_packet(p_t2, p_tile, l_tcp, l_current_pi, l_current_data,
                                           &l_nb_bytes_read, p_max_len, l_pack_info, p_manager)) {
                    opj_pi_destroy(l_pi, l_nb_pocs);
                    opj_free(first_pass_failed);
                    return OPJ_FALSE;
                }

                l_img_comp = &(l_image->comps[l_current_pi->compno]);
                l_img_comp->resno_decoded = opj_uint_max(l_current_pi->resno,
                                            l_img_comp->resno_decoded);
            } else {
                l_nb_bytes_read = 0;
                if (! opj_t2_skip_packet(p_t2, p_tile, l_tcp, l_current_pi, l_current_data,
                                         &l_nb_bytes_read, p_max_len, l_pack_info, p_manager)) {
                    opj_pi_destroy(l_pi, l_nb_pocs);
                    opj_free(first_pass_failed);
                    return OPJ_FALSE;
                }
            }

            if (first_pass_failed[l_current_pi->compno]) {
                l_img_comp = &(l_image->comps[l_current_pi->compno]);
                if (l_img_comp->resno_decoded == 0) {
                    l_img_comp->resno_decoded =
                        p_tile->comps[l_current_pi->compno].minimum_num_resolutions - 1;
                }
            }

            l_current_data += l_nb_bytes_read;
            p_max_len -= l_nb_bytes_read;

            /* INDEX >> */
#ifdef TODO_MSD
            if (p_cstr_info) {
                opj_tile_info_v2_t *info_TL = &p_cstr_info->tile[p_tile_no];
                opj_packet_info_t *info_PK = &info_TL->packet[p_cstr_info->packno];
                tp_start_packno = 0;
                if (!p_cstr_info->packno) {
                    info_PK->start_pos = info_TL->end_header + 1;
                } else if (info_TL->packet[p_cstr_info->packno - 1].end_pos >=
                           (OPJ_INT32)
                           p_cstr_info->tile[p_tile_no].tp[curtp].tp_end_pos) { /* New tile part */
                    info_TL->tp[curtp].tp_numpacks = p_cstr_info->packno -
                                                     tp_start_packno; /* Number of packets in previous tile-part */
                    tp_start_packno = p_cstr_info->packno;
                    curtp++;
                    info_PK->start_pos = p_cstr_info->tile[p_tile_no].tp[curtp].tp_end_header + 1;
                } else {
                    info_PK->start_pos = (l_cp->m_specific_param.m_enc.m_tp_on &&
                                          info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[p_cstr_info->packno -
                                                                      1].end_pos + 1;
                }
                info_PK->end_pos = info_PK->start_pos + l_nb_bytes_read - 1;
                info_PK->end_ph_pos += info_PK->start_pos -
                                       1;  /* End of packet header which now only represents the distance */
                ++p_cstr_info->packno;
            }
#endif
            /* << INDEX */
        }
        ++l_current_pi;

        opj_free(first_pass_failed);
    }
    /* INDEX >> */
#ifdef TODO_MSD
    if
    (p_cstr_info) {
        p_cstr_info->tile[p_tile_no].tp[curtp].tp_numpacks = p_cstr_info->packno -
                tp_start_packno; /* Number of packets in last tile-part */
    }
#endif
    /* << INDEX */

    /* don't forget to release pi */
    opj_pi_destroy(l_pi, l_nb_pocs);
    *p_data_read = (OPJ_UINT32)(l_current_data - p_src);
    return OPJ_TRUE;
}