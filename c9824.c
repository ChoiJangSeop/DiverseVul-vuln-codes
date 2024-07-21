OPJ_BOOL opj_t2_encode_packets(opj_t2_t* p_t2,
                               OPJ_UINT32 p_tile_no,
                               opj_tcd_tile_t *p_tile,
                               OPJ_UINT32 p_maxlayers,
                               OPJ_BYTE *p_dest,
                               OPJ_UINT32 * p_data_written,
                               OPJ_UINT32 p_max_len,
                               opj_codestream_info_t *cstr_info,
                               opj_tcd_marker_info_t* p_marker_info,
                               OPJ_UINT32 p_tp_num,
                               OPJ_INT32 p_tp_pos,
                               OPJ_UINT32 p_pino,
                               J2K_T2_MODE p_t2_mode,
                               opj_event_mgr_t *p_manager)
{
    OPJ_BYTE *l_current_data = p_dest;
    OPJ_UINT32 l_nb_bytes = 0;
    OPJ_UINT32 compno;
    OPJ_UINT32 poc;
    opj_pi_iterator_t *l_pi = 00;
    opj_pi_iterator_t *l_current_pi = 00;
    opj_image_t *l_image = p_t2->image;
    opj_cp_t *l_cp = p_t2->cp;
    opj_tcp_t *l_tcp = &l_cp->tcps[p_tile_no];
    OPJ_UINT32 pocno = (l_cp->rsiz == OPJ_PROFILE_CINEMA_4K) ? 2 : 1;
    OPJ_UINT32 l_max_comp = l_cp->m_specific_param.m_enc.m_max_comp_size > 0 ?
                            l_image->numcomps : 1;
    OPJ_UINT32 l_nb_pocs = l_tcp->numpocs + 1;

    l_pi = opj_pi_initialise_encode(l_image, l_cp, p_tile_no, p_t2_mode);
    if (!l_pi) {
        return OPJ_FALSE;
    }

    * p_data_written = 0;

    if (p_t2_mode == THRESH_CALC) { /* Calculating threshold */
        l_current_pi = l_pi;

        for (compno = 0; compno < l_max_comp; ++compno) {
            OPJ_UINT32 l_comp_len = 0;
            l_current_pi = l_pi;

            for (poc = 0; poc < pocno ; ++poc) {
                OPJ_UINT32 l_tp_num = compno;

                /* TODO MSD : check why this function cannot fail (cf. v1) */
                opj_pi_create_encode(l_pi, l_cp, p_tile_no, poc, l_tp_num, p_tp_pos, p_t2_mode);

                if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
                    /* TODO ADE : add an error */
                    opj_pi_destroy(l_pi, l_nb_pocs);
                    return OPJ_FALSE;
                }
                while (opj_pi_next(l_current_pi)) {
                    if (l_current_pi->layno < p_maxlayers) {
                        l_nb_bytes = 0;

                        if (! opj_t2_encode_packet(p_tile_no, p_tile, l_tcp, l_current_pi,
                                                   l_current_data, &l_nb_bytes,
                                                   p_max_len, cstr_info,
                                                   p_t2_mode,
                                                   p_manager)) {
                            opj_pi_destroy(l_pi, l_nb_pocs);
                            return OPJ_FALSE;
                        }

                        l_comp_len += l_nb_bytes;
                        l_current_data += l_nb_bytes;
                        p_max_len -= l_nb_bytes;

                        * p_data_written += l_nb_bytes;
                    }
                }

                if (l_cp->m_specific_param.m_enc.m_max_comp_size) {
                    if (l_comp_len > l_cp->m_specific_param.m_enc.m_max_comp_size) {
                        opj_pi_destroy(l_pi, l_nb_pocs);
                        return OPJ_FALSE;
                    }
                }

                ++l_current_pi;
            }
        }
    } else { /* t2_mode == FINAL_PASS  */
        opj_pi_create_encode(l_pi, l_cp, p_tile_no, p_pino, p_tp_num, p_tp_pos,
                             p_t2_mode);

        l_current_pi = &l_pi[p_pino];
        if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
            /* TODO ADE : add an error */
            opj_pi_destroy(l_pi, l_nb_pocs);
            return OPJ_FALSE;
        }

        if (p_marker_info && p_marker_info->need_PLT) {
            /* One time use intended */
            assert(p_marker_info->packet_count == 0);
            assert(p_marker_info->p_packet_size == NULL);

            p_marker_info->p_packet_size = (OPJ_UINT32*) opj_malloc(
                                               opj_get_encoding_packet_count(l_image, l_cp, p_tile_no) * sizeof(OPJ_UINT32));
            if (p_marker_info->p_packet_size == NULL) {
                opj_pi_destroy(l_pi, l_nb_pocs);
                return OPJ_FALSE;
            }
        }

        while (opj_pi_next(l_current_pi)) {
            if (l_current_pi->layno < p_maxlayers) {
                l_nb_bytes = 0;

                if (! opj_t2_encode_packet(p_tile_no, p_tile, l_tcp, l_current_pi,
                                           l_current_data, &l_nb_bytes, p_max_len,
                                           cstr_info, p_t2_mode, p_manager)) {
                    opj_pi_destroy(l_pi, l_nb_pocs);
                    return OPJ_FALSE;
                }

                l_current_data += l_nb_bytes;
                p_max_len -= l_nb_bytes;

                * p_data_written += l_nb_bytes;

                if (p_marker_info && p_marker_info->need_PLT) {
                    p_marker_info->p_packet_size[p_marker_info->packet_count] = l_nb_bytes;
                    p_marker_info->packet_count ++;
                }

                /* INDEX >> */
                if (cstr_info) {
                    if (cstr_info->index_write) {
                        opj_tile_info_t *info_TL = &cstr_info->tile[p_tile_no];
                        opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
                        if (!cstr_info->packno) {
                            info_PK->start_pos = info_TL->end_header + 1;
                        } else {
                            info_PK->start_pos = ((l_cp->m_specific_param.m_enc.m_tp_on | l_tcp->POC) &&
                                                  info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno -
                                                                            1].end_pos + 1;
                        }
                        info_PK->end_pos = info_PK->start_pos + l_nb_bytes - 1;
                        info_PK->end_ph_pos += info_PK->start_pos -
                                               1;  /* End of packet header which now only represents the distance
                                                                                                                                                                                                                                                   to start of packet is incremented by value of start of packet*/
                    }

                    cstr_info->packno++;
                }
                /* << INDEX */
                ++p_tile->packno;
            }
        }
    }

    opj_pi_destroy(l_pi, l_nb_pocs);

    return OPJ_TRUE;
}