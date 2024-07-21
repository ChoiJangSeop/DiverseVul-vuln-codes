int HevcSpsUnit::deserialize()
{
    int rez = HevcUnit::deserialize();
    if (rez)
        return rez;
    try
    {
        vps_id = m_reader.getBits(4);
        int max_sub_layers_minus1 = m_reader.getBits(3);
        if (max_sub_layers_minus1 > 6)
            return 1;
        max_sub_layers = max_sub_layers_minus1 + 1;
        int temporal_id_nesting_flag = m_reader.getBit();
        profile_tier_level(max_sub_layers);
        sps_id = extractUEGolombCode();
        if (sps_id > 15)
            return 1;
        chromaFormat = extractUEGolombCode();
        if (chromaFormat > 3)
            return 1;
        if (chromaFormat == 3)
            separate_colour_plane_flag = m_reader.getBit();
        pic_width_in_luma_samples = extractUEGolombCode();
        if (pic_width_in_luma_samples == 0)
            return 1;
        pic_height_in_luma_samples = extractUEGolombCode();
        if (pic_height_in_luma_samples == 0)
            return 1;
        if (pic_width_in_luma_samples >= 3840)
            V3_flags |= FOUR_K;

        bool conformance_window_flag = m_reader.getBit();
        if (conformance_window_flag)
        {
            extractUEGolombCode();  // conf_win_left_offset ue(v)
            extractUEGolombCode();  // conf_win_right_offset ue(v)
            extractUEGolombCode();  // conf_win_top_offset ue(v)
            extractUEGolombCode();  // conf_win_bottom_offset ue(v)
        }

        bit_depth_luma_minus8 = extractUEGolombCode();
        if (bit_depth_luma_minus8 > 8)
            return 1;
        bit_depth_chroma_minus8 = extractUEGolombCode();
        if (bit_depth_chroma_minus8 > 8)
            return 1;
        log2_max_pic_order_cnt_lsb = extractUEGolombCode() + 4;
        if (log2_max_pic_order_cnt_lsb > 16)
            return 1;
        bool sps_sub_layer_ordering_info_present_flag = m_reader.getBit();
        for (int i = (sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers - 1); i <= max_sub_layers - 1; i++)
        {
            unsigned sps_max_dec_pic_buffering_minus1 = extractUEGolombCode();
            unsigned sps_max_num_reorder_pics = extractUEGolombCode();
            if (sps_max_num_reorder_pics > sps_max_dec_pic_buffering_minus1)
                return 1;
            unsigned sps_max_latency_increase_plus1 = extractUEGolombCode();
            if (sps_max_latency_increase_plus1 == 0xffffffff)
                return 1;
        }

        unsigned log2_min_luma_coding_block_size_minus3 = extractUEGolombCode();
        unsigned log2_diff_max_min_luma_coding_block_size = extractUEGolombCode();
        extractUEGolombCode();  // log2_min_luma_transform_block_size_minus2 ue(v)
        extractUEGolombCode();  // log2_diff_max_min_luma_transform_block_size ue(v)
        extractUEGolombCode();  // max_transform_hierarchy_depth_inter ue(v)
        extractUEGolombCode();  // max_transform_hierarchy_depth_intra ue(v)

        int MinCbLog2SizeY = log2_min_luma_coding_block_size_minus3 + 3;
        int CtbLog2SizeY = MinCbLog2SizeY + log2_diff_max_min_luma_coding_block_size;
        int CtbSizeY = 1 << CtbLog2SizeY;
        int PicWidthInCtbsY = ceilDiv(pic_width_in_luma_samples, CtbSizeY);
        int PicHeightInCtbsY = ceilDiv(pic_height_in_luma_samples, CtbSizeY);
        int PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;
        PicSizeInCtbsY_bits = 0;
        int count1bits = 0;
        while (PicSizeInCtbsY)
        {
            count1bits += PicSizeInCtbsY & 1;
            PicSizeInCtbsY_bits++;
            PicSizeInCtbsY >>= 1;
        }
        if (count1bits == 1)
            PicSizeInCtbsY_bits -= 1;  // in case PicSizeInCtbsY is power of 2

        bool scaling_list_enabled_flag = m_reader.getBit();
        if (scaling_list_enabled_flag)
        {
            bool sps_scaling_list_data_present_flag = m_reader.getBit();
            if (sps_scaling_list_data_present_flag)
                if (scaling_list_data())
                    return 1;
        }

        m_reader.skipBit();  // amp_enabled_flag u(1)
        m_reader.skipBit();  // sample_adaptive_offset_enabled_flag u(1)
        bool pcm_enabled_flag = m_reader.getBit();
        if (pcm_enabled_flag)
        {
            m_reader.skipBits(4);  // pcm_sample_bit_depth_luma_minus1 u(4)
            m_reader.skipBits(4);  // pcm_sample_bit_depth_chroma_minus1 u(4)
            unsigned log2_min_pcm_luma_coding_block_size_minus3 = extractUEGolombCode();
            if (log2_min_pcm_luma_coding_block_size_minus3 > 2)
                return 1;
            unsigned log2_diff_max_min_pcm_luma_coding_block_size = extractUEGolombCode();
            if (log2_diff_max_min_pcm_luma_coding_block_size > 2)
                return 1;
            m_reader.skipBit();  // m_rpcm_loop_filter_disabled_flag u(1)
        }
        num_short_term_ref_pic_sets = extractUEGolombCode();
        if (num_short_term_ref_pic_sets > 64)
            return 1;
        /*
        NumNegativePics.resize(num_short_term_ref_pic_sets);
        NumPositivePics.resize(num_short_term_ref_pic_sets);
        UsedByCurrPicS0.resize(num_short_term_ref_pic_sets);
        UsedByCurrPicS1.resize(num_short_term_ref_pic_sets);
        DeltaPocS0.resize(num_short_term_ref_pic_sets);
        DeltaPocS1.resize(num_short_term_ref_pic_sets);
        for (int i = 0; i < num_short_term_ref_pic_sets; ++i) {
            UsedByCurrPicS0[i].resize(MAX_REFS);
            DeltaPocS0[i].resize(MAX_REFS);
            UsedByCurrPicS1[i].resize(MAX_REFS);
            DeltaPocS1[i].resize(MAX_REFS);
        }
        */
        st_rps.resize(num_short_term_ref_pic_sets);

        for (size_t i = 0; i < num_short_term_ref_pic_sets; i++)
            if (short_term_ref_pic_set(i) != 0)
                return 1;
        bool long_term_ref_pics_present_flag = m_reader.getBit();
        if (long_term_ref_pics_present_flag)
        {
            unsigned num_long_term_ref_pics_sps = extractUEGolombCode();
            if (num_long_term_ref_pics_sps > 32)
                return 1;
            for (size_t i = 0; i < num_long_term_ref_pics_sps; i++)
            {
                m_reader.skipBits(log2_max_pic_order_cnt_lsb);  // lt_ref_pic_poc_lsb_sps[ i ] u(v)
                m_reader.skipBit();                             // used_by_curr_pic_lt_sps_flag[ i ] u(1)
            }
        }
        m_reader.skipBit();  // sps_temporal_mvp_enabled_flag u(1)
        m_reader.skipBit();  // strong_intra_smoothing_enabled_flag u(1)
        bool vui_parameters_present_flag = m_reader.getBit();
        if (vui_parameters_present_flag)
            if (vui_parameters())
                return 1;
        bool sps_extension_flag = m_reader.getBit();

        int gg = m_reader.getBitsLeft();
        gg = gg;

        return 0;
    }
    catch (VodCoreException& e)
    {
        return NOT_ENOUGH_BUFFER;
    }
}