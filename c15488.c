int VvcSpsUnit::deserialize()
{
    int rez = VvcUnit::deserialize();
    if (rez)
        return rez;
    try
    {
        sps_id = m_reader.getBits(4);
        vps_id = m_reader.getBits(4);
        max_sublayers_minus1 = m_reader.getBits(3);
        if (max_sublayers_minus1 == 7)
            return 1;
        chroma_format_idc = m_reader.getBits(2);
        unsigned sps_log2_ctu_size_minus5 = m_reader.getBits(2);
        if (sps_log2_ctu_size_minus5 > 2)
            return 1;
        int CtbLog2SizeY = sps_log2_ctu_size_minus5 + 5;
        unsigned CtbSizeY = 1 << CtbLog2SizeY;
        bool sps_ptl_dpb_hrd_params_present_flag = m_reader.getBit();
        if (sps_id == 0 && !sps_ptl_dpb_hrd_params_present_flag)
            return 1;
        if (sps_ptl_dpb_hrd_params_present_flag)
            profile_tier_level(1, max_sublayers_minus1);
        m_reader.skipBit();      // sps_gdr_enabled_flag
        if (m_reader.getBit())   // sps_ref_pic_resampling_enabled_flag
            m_reader.skipBit();  // sps_res_change_in_clvs_allowed_flag
        pic_width_max_in_luma_samples = extractUEGolombCode();
        pic_height_max_in_luma_samples = extractUEGolombCode();
        unsigned tmpWidthVal = (pic_width_max_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
        unsigned tmpHeightVal = (pic_height_max_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
        unsigned sps_conf_win_left_offset = 0;
        unsigned sps_conf_win_right_offset = 0;
        unsigned sps_conf_win_top_offset = 0;
        unsigned sps_conf_win_bottom_offset = 0;
        if (m_reader.getBit())  // sps_conformance_window_flag
        {
            sps_conf_win_left_offset = extractUEGolombCode();
            sps_conf_win_right_offset = extractUEGolombCode();
            sps_conf_win_top_offset = extractUEGolombCode();
            sps_conf_win_bottom_offset = extractUEGolombCode();
        }
        if (m_reader.getBit())  // sps_subpic_info_present_flag
        {
            unsigned sps_num_subpics_minus1 = extractUEGolombCode();
            if (sps_num_subpics_minus1 > 600)
                return 1;
            if (sps_num_subpics_minus1 > 0)
            {
                bool sps_independent_subpics_flag = m_reader.getBit();
                bool sps_subpic_same_size_flag = m_reader.getBit();
                for (size_t i = 0; i <= sps_num_subpics_minus1; i++)
                {
                    if (!sps_subpic_same_size_flag || i == 0)
                    {
                        if (i != 0 && pic_width_max_in_luma_samples > CtbSizeY)
                            m_reader.skipBits(ceil(log2(tmpWidthVal)));  // sps_subpic_ctu_top_left_x[i]
                        if (i != 0 && pic_height_max_in_luma_samples > CtbSizeY)
                            m_reader.skipBits(ceil(log2(tmpHeightVal)));  // sps_subpic_ctu_top_left_y[i]
                        if (i < sps_num_subpics_minus1 && pic_width_max_in_luma_samples > CtbSizeY)
                            m_reader.skipBits(ceil(log2(tmpWidthVal)));  // sps_subpic_width_minus1[i]
                        if (i < sps_num_subpics_minus1 && pic_height_max_in_luma_samples > CtbSizeY)
                            m_reader.skipBits(ceil(log2(tmpHeightVal)));  // sps_subpic_height_minus1[i]
                    }
                    if (!sps_independent_subpics_flag)
                    {
                        m_reader.skipBit();  // sps_subpic_treated_as_pic_flag
                        m_reader.skipBit();  // sps_loop_filter_across_subpic_enabled_flag
                    }
                }
            }
            unsigned sps_subpic_id_len = extractUEGolombCode() + 1;
            if (sps_subpic_id_len > 16 || (unsigned)(1 << sps_subpic_id_len) < (sps_num_subpics_minus1 + 1))
                return 1;
            if (m_reader.getBit())  // sps_subpic_id_mapping_explicitly_signalled_flag
            {
                if (m_reader.getBit())  // sps_subpic_id_mapping_present_flag
                    for (size_t i = 0; i <= sps_num_subpics_minus1; i++)
                        m_reader.skipBits(sps_subpic_id_len);  // sps_subpic_id[i]
            }
        }
        bitdepth_minus8 = extractUEGolombCode();
        if (bitdepth_minus8 > 2)
            return 1;
        int QpBdOffset = 6 * bitdepth_minus8;
        m_reader.skipBit();  // sps_entropy_coding_sync_enabled_flag
        m_reader.skipBit();  // vsps_entry_point_offsets_present_flag
        log2_max_pic_order_cnt_lsb = m_reader.getBits(4) + 4;
        if (log2_max_pic_order_cnt_lsb > 16)
            return 1;
        if (m_reader.getBit())  // sps_poc_msb_cycle_flag
        {
            if (extractUEGolombCode() /* sps_poc_msb_cycle_len_minus1 */ > 23 - log2_max_pic_order_cnt_lsb)
                return 1;
        }
        int sps_num_extra_ph_bytes = m_reader.getBits(2);
        for (size_t i = 0; i < sps_num_extra_ph_bytes; i++) m_reader.skipBits(8);  // sps_extra_ph_bit_present_flag[i]
        int sps_num_extra_sh_bytes = m_reader.getBits(2);
        for (size_t i = 0; i < sps_num_extra_sh_bytes; i++) m_reader.skipBits(8);  // sps_extra_sh_bit_present_flag[i]
        if (sps_ptl_dpb_hrd_params_present_flag)
        {
            bool sps_sublayer_dpb_params_flag = (max_sublayers_minus1 > 0) ? m_reader.getBit() : 0;
            if (dpb_parameters(max_sublayers_minus1, sps_sublayer_dpb_params_flag))
                return 1;
        }
        unsigned sps_log2_min_luma_coding_block_size_minus2 = extractUEGolombCode();
        if (sps_log2_min_luma_coding_block_size_minus2 > (unsigned)min(4, (int)sps_log2_ctu_size_minus5 + 3))
            return 1;
        unsigned MinCbLog2SizeY = sps_log2_min_luma_coding_block_size_minus2 + 2;
        unsigned MinCbSizeY = 1 << MinCbLog2SizeY;
        m_reader.skipBit();  // sps_partition_constraints_override_enabled_flag
        unsigned sps_log2_diff_min_qt_min_cb_intra_slice_luma = extractUEGolombCode();
        if (sps_log2_diff_min_qt_min_cb_intra_slice_luma > min(6, CtbLog2SizeY) - MinCbLog2SizeY)
            return 1;
        unsigned MinQtLog2SizeIntraY = sps_log2_diff_min_qt_min_cb_intra_slice_luma + MinCbLog2SizeY;
        unsigned sps_max_mtt_hierarchy_depth_intra_slice_luma = extractUEGolombCode();
        if (sps_max_mtt_hierarchy_depth_intra_slice_luma > 2 * (CtbLog2SizeY - MinCbLog2SizeY))
            return 1;
        if (sps_max_mtt_hierarchy_depth_intra_slice_luma != 0)
        {
            if (extractUEGolombCode() >
                CtbLog2SizeY - MinQtLog2SizeIntraY)  // sps_log2_diff_max_bt_min_qt_intra_slice_luma
                return 1;
            if (extractUEGolombCode() >
                min(6, CtbLog2SizeY) - MinQtLog2SizeIntraY)  // sps_log2_diff_max_tt_min_qt_intra_slice_luma
                return 1;
        }
        bool sps_qtbtt_dual_tree_intra_flag = (chroma_format_idc != 0 ? m_reader.getBit() : 0);
        if (sps_qtbtt_dual_tree_intra_flag)
        {
            unsigned sps_log2_diff_min_qt_min_cb_intra_slice_chroma = extractUEGolombCode();
            if (sps_log2_diff_min_qt_min_cb_intra_slice_chroma > min(6, CtbLog2SizeY) - MinCbLog2SizeY)  //
                return 1;
            unsigned MinQtLog2SizeIntraC = sps_log2_diff_min_qt_min_cb_intra_slice_chroma + MinCbLog2SizeY;
            unsigned sps_max_mtt_hierarchy_depth_intra_slice_chroma = extractUEGolombCode();
            if (sps_max_mtt_hierarchy_depth_intra_slice_chroma > 2 * (CtbLog2SizeY - MinCbLog2SizeY))
                return 1;
            if (sps_max_mtt_hierarchy_depth_intra_slice_chroma != 0)
            {
                if (extractUEGolombCode() >
                    min(6, CtbLog2SizeY) - MinQtLog2SizeIntraC)  // sps_log2_diff_max_bt_min_qt_intra_slice_chroma
                    return 1;
                if (extractUEGolombCode() >
                    min(6, CtbLog2SizeY) - MinQtLog2SizeIntraC)  // sps_log2_diff_max_tt_min_qt_intra_slice_chroma
                    return 1;
            }
        }
        unsigned sps_log2_diff_min_qt_min_cb_inter_slice = extractUEGolombCode();
        if (sps_log2_diff_min_qt_min_cb_inter_slice > min(6, CtbLog2SizeY) - MinCbLog2SizeY)
            return 1;
        unsigned MinQtLog2SizeInterY = sps_log2_diff_min_qt_min_cb_inter_slice + MinCbLog2SizeY;
        unsigned sps_max_mtt_hierarchy_depth_inter_slice = extractUEGolombCode();
        if (sps_max_mtt_hierarchy_depth_inter_slice > 2 * (CtbLog2SizeY - MinCbLog2SizeY))
            return 1;
        if (sps_max_mtt_hierarchy_depth_inter_slice != 0)
        {
            if (extractUEGolombCode() > CtbLog2SizeY - MinQtLog2SizeInterY)  // sps_log2_diff_max_bt_min_qt_inter_slice
                return 1;
            if (extractUEGolombCode() >
                min(6, CtbLog2SizeY) - MinQtLog2SizeInterY)  // sps_log2_diff_max_tt_min_qt_inter_slice
                return 1;
        }
        bool sps_max_luma_transform_size_64_flag = (CtbSizeY > 32 ? m_reader.getBit() : 0);
        bool sps_transform_skip_enabled_flag = m_reader.getBit();
        if (sps_transform_skip_enabled_flag)
        {
            if (extractUEGolombCode() > 3)  // sps_log2_transform_skip_max_size_minus2
                return 1;
            m_reader.skipBit();  // sps_bdpcm_enabled_flag
        }
        if (m_reader.getBit())  // sps_mts_enabled_flag
        {
            m_reader.skipBit();  // sps_explicit_mts_intra_enabled_flag
            m_reader.skipBit();  // sps_explicit_mts_inter_enabled_flag
        }
        bool sps_lfnst_enabled_flag = m_reader.getBit();
        if (chroma_format_idc != 0)
        {
            bool sps_joint_cbcr_enabled_flag = m_reader.getBit();
            int numQpTables =
                m_reader.getBit() /* sps_same_qp_table_for_chroma_flag */ ? 1 : (sps_joint_cbcr_enabled_flag ? 3 : 2);
            for (int i = 0; i < numQpTables; i++)
            {
                int sps_qp_table_start_minus26 = extractSEGolombCode();
                if (sps_qp_table_start_minus26 < (-26 - QpBdOffset) || sps_qp_table_start_minus26 > 36)
                    return 1;
                unsigned sps_num_points_in_qp_table_minus1 = extractUEGolombCode();
                if (sps_num_points_in_qp_table_minus1 > (unsigned)(36 - sps_qp_table_start_minus26))
                    return 1;
                for (size_t j = 0; j <= sps_num_points_in_qp_table_minus1; j++)
                {
                    extractUEGolombCode();  // sps_delta_qp_in_val_minus1
                    extractUEGolombCode();  // sps_delta_qp_diff_val
                }
            }
        }
        m_reader.skipBit();  // sps_sao_enabled_flag
        if (m_reader.getBit() /* sps_alf_enabled_flag */ && chroma_format_idc != 0)
            m_reader.skipBit();  // sps_ccalf_enabled_flag
        m_reader.skipBit();      // sps_lmcs_enabled_flag
        weighted_pred_flag = m_reader.getBit();
        weighted_bipred_flag = m_reader.getBit();
        long_term_ref_pics_flag = m_reader.getBit();
        inter_layer_prediction_enabled_flag = (sps_id != 0) ? m_reader.getBit() : 0;
        m_reader.skipBit();  // sps_idr_rpl_present_flag
        bool sps_rpl1_same_as_rpl0_flag = m_reader.getBit();
        for (size_t i = 0; i < (sps_rpl1_same_as_rpl0_flag ? 1 : 2); i++)
        {
            sps_num_ref_pic_lists = extractUEGolombCode();
            if (sps_num_ref_pic_lists > 64)
                return 1;
            for (size_t j = 0; j < sps_num_ref_pic_lists; j++) ref_pic_list_struct(i, j);
        }
        m_reader.skipBit();  // sps_ref_wraparound_enabled_flag
        bool sps_sbtmvp_enabled_flag = (m_reader.getBit()) /* sps_temporal_mvp_enabled_flag */ ? m_reader.getBit() : 0;
        bool sps_amvr_enabled_flag = m_reader.getBit();
        if (m_reader.getBit())   // sps_bdof_enabled_flag
            m_reader.skipBit();  // sps_bdof_control_present_in_ph_flag
        m_reader.skipBit();      // sps_smvd_enabled_flag
        if (m_reader.getBit())   // sps_dmvr_enabled_flag
            m_reader.skipBit();  // sps_dmvr_control_present_in_ph_flag
        if (m_reader.getBit())   // sps_mmvd_enabled_flag
            m_reader.skipBit();  // sps_mmvd_fullpel_only_enabled_flag
        unsigned sps_six_minus_max_num_merge_cand = extractUEGolombCode();
        if (sps_six_minus_max_num_merge_cand > 5)
            return 1;
        unsigned MaxNumMergeCand = 6 - sps_six_minus_max_num_merge_cand;
        m_reader.skipBit();     // sps_sbt_enabled_flag
        if (m_reader.getBit())  // sps_affine_enabled_flag
        {
            unsigned sps_five_minus_max_num_subblock_merge_cand = extractUEGolombCode();
            if (sps_five_minus_max_num_subblock_merge_cand + sps_sbtmvp_enabled_flag > 5)
                return 1;
            m_reader.skipBit();  // sps_6param_affine_enabled_flag
            if (sps_amvr_enabled_flag)
                m_reader.skipBit();  // sps_affine_amvr_enabled_flag
            if (m_reader.getBit())   // sps_affine_prof_enabled_flag
                m_reader.skipBit();  // sps_prof_control_present_in_ph_flag
        }
        m_reader.skipBit();  // sps_bcw_enabled_flag
        m_reader.skipBit();  // sps_ciip_enabled_flag
        if (MaxNumMergeCand >= 2)
        {
            if (m_reader.getBit() /* sps_gpm_enabled_flag */ && MaxNumMergeCand >= 3)
            {
                unsigned sps_max_num_merge_cand_minus_max_num_gpm_cand = extractUEGolombCode();
                if (sps_max_num_merge_cand_minus_max_num_gpm_cand + 2 > MaxNumMergeCand)
                    return 1;
            }
        }
        unsigned sps_log2_parallel_merge_level_minus2 = extractUEGolombCode();
        if (sps_log2_parallel_merge_level_minus2 > CtbLog2SizeY - 2)
            return 1;

        bool sps_isp_enabled_flag = m_reader.getBit();
        bool sps_mrl_enabled_flag = m_reader.getBit();
        bool sps_mip_enabled_flag = m_reader.getBit();
        if (chroma_format_idc != 0)
            bool sps_cclm_enabled_flag = m_reader.getBit();
        if (chroma_format_idc == 1)
        {
            bool sps_chroma_horizontal_collocated_flag = m_reader.getBit();
            bool sps_chroma_vertical_collocated_flag = m_reader.getBit();
        }
        bool sps_palette_enabled_flag = m_reader.getBit();
        bool sps_act_enabled_flag =
            (chroma_format_idc == 3 && !sps_max_luma_transform_size_64_flag) ? m_reader.getBit() : 0;
        if (sps_transform_skip_enabled_flag || sps_palette_enabled_flag)
        {
            unsigned sps_min_qp_prime_ts = extractUEGolombCode();
            if (sps_min_qp_prime_ts > 8)
                return 1;
        }
        if (m_reader.getBit())  // sps_ibc_enabled_flag
        {
            unsigned sps_six_minus_max_num_ibc_merge_cand = extractUEGolombCode();
            if (sps_six_minus_max_num_ibc_merge_cand > 5)
                return 1;
        }
        if (m_reader.getBit())  // sps_ladf_enabled_flag
        {
            int sps_num_ladf_intervals_minus2 = m_reader.getBits(2);
            int sps_ladf_lowest_interval_qp_offset = extractSEGolombCode();
            for (int i = 0; i < sps_num_ladf_intervals_minus2 + 1; i++)
            {
                int sps_ladf_qp_offset = extractSEGolombCode();
                unsigned sps_ladf_delta_threshold_minus1 = extractUEGolombCode();
            }
        }
        bool sps_explicit_scaling_list_enabled_flag = m_reader.getBit();
        if (sps_lfnst_enabled_flag && sps_explicit_scaling_list_enabled_flag)
            bool sps_scaling_matrix_for_lfnst_disabled_flag = m_reader.getBit();
        bool sps_scaling_matrix_for_alternative_colour_space_disabled_flag =
            (sps_act_enabled_flag && sps_explicit_scaling_list_enabled_flag) ? m_reader.getBit() : 0;
        if (sps_scaling_matrix_for_alternative_colour_space_disabled_flag)
            bool sps_scaling_matrix_designated_colour_space_flag = m_reader.getBit();
        bool sps_dep_quant_enabled_flag = m_reader.getBit();
        bool sps_sign_data_hiding_enabled_flag = m_reader.getBit();
        if (m_reader.getBit())  // sps_virtual_boundaries_enabled_flag
        {
            if (m_reader.getBit())  // sps_virtual_boundaries_present_flag
            {
                unsigned sps_num_ver_virtual_boundaries = extractUEGolombCode();
                if (sps_num_ver_virtual_boundaries > (pic_width_max_in_luma_samples <= 8 ? 0 : 3))
                    return 1;
                for (size_t i = 0; i < sps_num_ver_virtual_boundaries; i++)
                {
                    unsigned sps_virtual_boundary_pos_x_minus1 = extractUEGolombCode();
                    if (sps_virtual_boundary_pos_x_minus1 > ceil(pic_width_max_in_luma_samples / 8) - 2)
                        return 1;
                }
                unsigned sps_num_hor_virtual_boundaries = extractUEGolombCode();
                if (sps_num_hor_virtual_boundaries > (pic_height_max_in_luma_samples <= 8 ? 0 : 3))
                    return 1;
                for (size_t i = 0; i < sps_num_hor_virtual_boundaries; i++)
                {
                    unsigned sps_virtual_boundary_pos_y_minus1 = extractUEGolombCode();
                    if (sps_virtual_boundary_pos_y_minus1 > ceil(pic_height_max_in_luma_samples / 8) - 2)
                        return 1;
                }
            }
        }
        if (sps_ptl_dpb_hrd_params_present_flag)
        {
            if (m_reader.getBit())  // sps_timing_hrd_params_present_flag
            {
                if (m_sps_hrd.general_timing_hrd_parameters())
                    return 1;
                int sps_sublayer_cpb_params_present_flag = (max_sublayers_minus1 > 0) ? m_reader.getBit() : 0;
                int firstSubLayer = sps_sublayer_cpb_params_present_flag ? 0 : max_sublayers_minus1;
                m_sps_hrd.ols_timing_hrd_parameters(firstSubLayer, max_sublayers_minus1);
            }
        }
        bool sps_field_seq_flag = m_reader.getBit();
        if (m_reader.getBit())  // sps_vui_parameters_present_flag
        {
            unsigned sps_vui_payload_size_minus1 = extractUEGolombCode();
            if (sps_vui_payload_size_minus1 > 1023)
                return 1;
            m_reader.skipBits(m_reader.getBitsLeft() % 8);  // sps_vui_alignment_zero_bit
            vui_parameters();
        }
        bool sps_extension_flag = m_reader.getBit();
        return 0;
    }
    catch (VodCoreException& e)
    {
        return NOT_ENOUGH_BUFFER;
    }
}