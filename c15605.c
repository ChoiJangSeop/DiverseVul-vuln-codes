int parse_sequence_header(davs2_mgr_t *mgr, davs2_seq_t *seq, davs2_bs_t *bs)
{
    static const float FRAME_RATE[8] = {
        24000.0f / 1001.0f, 24.0f, 25.0f, 30000.0f / 1001.0f, 30.0f, 50.0f, 60000.0f / 1001.0f, 60.0f
    };
    rps_t *p_rps      = NULL;

    int i, j;
    int num_of_rps;

    bs->i_bit_pos += 32; /* skip start code */

    memset(seq, 0, sizeof(davs2_seq_t));  // reset all value

    seq->head.profile_id       = u_v(bs, 8, "profile_id");
    seq->head.level_id         = u_v(bs, 8, "level_id");
    seq->head.progressive      = u_v(bs, 1, "progressive_sequence");
    seq->b_field_coding        = u_flag(bs, "field_coded_sequence");

    seq->head.width     = u_v(bs, 14, "horizontal_size");
    seq->head.height    = u_v(bs, 14, "vertical_size");

    if (seq->head.width < 16 || seq->head.height < 16) {
        return -1;
    }

    seq->head.chroma_format = u_v(bs, 2, "chroma_format");

    if (seq->head.chroma_format != CHROMA_420 && seq->head.chroma_format != CHROMA_400) {
        return -1;
    }
    if (seq->head.chroma_format == CHROMA_400) {
        davs2_log(mgr, DAVS2_LOG_WARNING, "Un-supported Chroma Format YUV400 as 0 for GB/T.\n");
    }

    /* sample bit depth */
    if (seq->head.profile_id == MAIN10_PROFILE) {
        seq->sample_precision      = u_v(bs, 3, "sample_precision");
        seq->encoding_precision    = u_v(bs, 3, "encoding_precision");
    } else {
        seq->sample_precision      = u_v(bs, 3, "sample_precision");
        seq->encoding_precision    = 1;
    }
    if (seq->sample_precision < 1 || seq->sample_precision > 3 ||
        seq->encoding_precision < 1 || seq->encoding_precision > 3) {
        return -1;
    }

    seq->head.internal_bit_depth   = 6 + (seq->encoding_precision << 1);
    seq->head.output_bit_depth     = 6 + (seq->encoding_precision << 1);
    seq->head.bytes_per_sample     = seq->head.output_bit_depth > 8 ? 2 : 1;

    /*  */
    seq->head.aspect_ratio         = u_v(bs, 4, "aspect_ratio_information");
    seq->head.frame_rate_id        = u_v(bs, 4, "frame_rate_id");
    seq->bit_rate_lower            = u_v(bs, 18, "bit_rate_lower");
    u_v(bs, 1,  "marker bit");
    seq->bit_rate_upper            = u_v(bs, 12, "bit_rate_upper");
    seq->head.low_delay            = u_v(bs, 1, "low_delay");
    u_v(bs, 1,  "marker bit");
    seq->b_temporal_id_exist       = u_flag(bs, "temporal_id exist flag"); // get Extension Flag
    u_v(bs, 18, "bbv buffer size");

    seq->log2_lcu_size             = u_v(bs, 3, "Largest Coding Block Size");

    if (seq->log2_lcu_size < 4 || seq->log2_lcu_size > 6) {
        davs2_log(mgr, DAVS2_LOG_ERROR, "Invalid LCU size: %d\n", seq->log2_lcu_size);
        return -1;
    }

    seq->enable_weighted_quant     = u_flag(bs, "enable_weighted_quant");

    if (seq->enable_weighted_quant) {
        int load_seq_wquant_data_flag;
        int x, y, sizeId, uiWqMSize;
        const int *Seq_WQM;

        load_seq_wquant_data_flag = u_flag(bs,  "load_seq_weight_quant_data_flag");

        for (sizeId = 0; sizeId < 2; sizeId++) {
            uiWqMSize = DAVS2_MIN(1 << (sizeId + 2), 8);
            if (load_seq_wquant_data_flag == 1) {
                for (y = 0; y < uiWqMSize; y++) {
                    for (x = 0; x < uiWqMSize; x++) {
                        seq->seq_wq_matrix[sizeId][y * uiWqMSize + x] = (int16_t)ue_v(bs, "weight_quant_coeff");
                    }
                }
            } else if (load_seq_wquant_data_flag == 0) {
                Seq_WQM = wq_get_default_matrix(sizeId);
                for (i = 0; i < (uiWqMSize * uiWqMSize); i++) {
                    seq->seq_wq_matrix[sizeId][i] = (int16_t)Seq_WQM[i];
                }
            }
        }
    }

    seq->enable_background_picture = u_flag(bs, "background_picture_disable") ^ 0x01;
    seq->enable_mhp_skip           = u_flag(bs, "mhpskip enabled");
    seq->enable_dhp                = u_flag(bs, "dhp enabled");
    seq->enable_wsm                = u_flag(bs, "wsm enabled");
    seq->enable_amp                = u_flag(bs, "Asymmetric Motion Partitions");
    seq->enable_nsqt               = u_flag(bs, "use NSQT");
    seq->enable_sdip               = u_flag(bs, "use NSIP");
    seq->enable_2nd_transform      = u_flag(bs, "secT enabled");
    seq->enable_sao                = u_flag(bs, "SAO Enable Flag");
    seq->enable_alf                = u_flag(bs, "ALF Enable Flag");
    seq->enable_pmvr               = u_flag(bs, "pmvr enabled");

    if (1 != u_v(bs, 1, "marker bit"))  {
        davs2_log(mgr, DAVS2_LOG_ERROR, "expected marker_bit 1 while received 0, FILE %s, Row %d\n", __FILE__, __LINE__);
    }

    num_of_rps                      = u_v(bs, 6, "num_of_RPS");
    if (num_of_rps > AVS2_GOP_NUM) {
        return -1;
    }

    seq->num_of_rps = num_of_rps;

    for (i = 0; i < num_of_rps; i++) {
        p_rps = &seq->seq_rps[i];

        p_rps->refered_by_others        = u_v(bs, 1,  "refered by others");
        p_rps->num_of_ref               = u_v(bs, 3,  "num of reference picture");

        for (j = 0; j < p_rps->num_of_ref; j++) {
            p_rps->ref_pic[j]           = u_v(bs, 6,  "delta COI of ref pic");
        }

        p_rps->num_to_remove            = u_v(bs, 3,  "num of removed picture");

        for (j = 0; j < p_rps->num_to_remove; j++) {
            p_rps->remove_pic[j]        = u_v(bs, 6,  "delta COI of removed pic");
        }

        if (1 != u_v(bs, 1, "marker bit"))  {
            davs2_log(mgr, DAVS2_LOG_ERROR, "expected marker_bit 1 while received 0, FILE %s, Row %d\n", __FILE__, __LINE__);
        }
    }

    if (seq->head.low_delay == 0) {
        seq->picture_reorder_delay = u_v(bs, 5, "picture_reorder_delay");
    }

    seq->cross_loop_filter_flag    = u_flag(bs, "Cross Loop Filter Flag");
    u_v(bs, 2,  "reserved bits");

    bs_align(bs); /* align position */

    seq->head.bitrate    = ((seq->bit_rate_upper << 18) + seq->bit_rate_lower) * 400;
    seq->head.frame_rate = FRAME_RATE[seq->head.frame_rate_id - 1];

    seq->i_enc_width     = ((seq->head.width + MIN_CU_SIZE - 1) >> MIN_CU_SIZE_IN_BIT) << MIN_CU_SIZE_IN_BIT;
    seq->i_enc_height    = ((seq->head.height   + MIN_CU_SIZE - 1) >> MIN_CU_SIZE_IN_BIT) << MIN_CU_SIZE_IN_BIT;
    seq->valid_flag = 1;

    return 0;
}