int VvcVpsUnit::deserialize()
{
    int rez = VvcUnit::deserialize();
    if (rez)
        return rez;
    try
    {
        vps_id = m_reader.getBits(4);
        vps_max_layers = m_reader.getBits(6) + 1;
        vps_max_sublayers = m_reader.getBits(3) + 1;
        bool vps_default_ptl_dpb_hrd_max_tid_flag =
            (vps_max_layers > 1 && vps_max_sublayers > 1) ? m_reader.getBit() : 1;
        int vps_all_independent_layers_flag = (vps_max_layers > 1) ? m_reader.getBit() : 1;
        for (int i = 0; i < vps_max_layers; i++)
        {
            m_reader.skipBits(6);  // vps_layer_id[i]
            if (i > 0 && !vps_all_independent_layers_flag)
            {
                if (!m_reader.getBit())  // vps_independent_layer_flag[i]
                {
                    bool vps_max_tid_ref_present_flag = m_reader.getBit();
                    for (int j = 0; j < i; j++)
                    {
                        bool vps_direct_ref_layer_flag = m_reader.getBit();
                        if (vps_max_tid_ref_present_flag && vps_direct_ref_layer_flag)
                            m_reader.skipBits(3);  // vps_max_tid_il_ref_pics_plus1[i][j]
                    }
                }
            }
        }

        bool vps_each_layer_is_an_ols_flag = 1;
        int vps_num_ptls = 1;
        int vps_ols_mode_idc = 2;
        int olsModeIdc = 4;
        int TotalNumOlss = vps_max_layers;
        if (vps_max_layers > 1)
        {
            vps_each_layer_is_an_ols_flag = 0;
            if (vps_all_independent_layers_flag)
                vps_each_layer_is_an_ols_flag = m_reader.getBit();
            if (!vps_each_layer_is_an_ols_flag)
            {
                if (!vps_all_independent_layers_flag)
                {
                    vps_ols_mode_idc = m_reader.getBits(2);
                    olsModeIdc = vps_ols_mode_idc;
                }
                if (vps_ols_mode_idc == 2)
                {
                    int vps_num_output_layer_sets_minus2 = m_reader.getBits(8);
                    TotalNumOlss = vps_num_output_layer_sets_minus2 + 2;
                    for (int i = 1; i <= vps_num_output_layer_sets_minus2 + 1; i++)
                        for (int j = 0; j < vps_max_layers; j++) m_reader.skipBit();  // vps_ols_output_layer_flag[i][j]
                }
            }
            vps_num_ptls = m_reader.getBits(8) + 1;
        }

        std::vector<bool> vps_pt_present_flag;
        std::vector<int> vps_ptl_max_tid;
        vps_pt_present_flag.resize(vps_num_ptls);
        vps_ptl_max_tid.resize(vps_num_ptls);

        for (int i = 0; i < vps_num_ptls; i++)
        {
            if (i > 0)
                vps_pt_present_flag[i] = m_reader.getBit();
            if (!vps_default_ptl_dpb_hrd_max_tid_flag)
                vps_ptl_max_tid[i] = m_reader.getBits(3);
        }

        m_reader.skipBits(m_reader.getBitsLeft() % 8);  // vps_ptl_alignment_zero_bit

        for (int i = 0; i < vps_num_ptls; i++) profile_tier_level(vps_pt_present_flag[i], vps_ptl_max_tid[i]);

        for (int i = 0; i < TotalNumOlss; i++)
            if (vps_num_ptls > 1 && vps_num_ptls != TotalNumOlss)
                m_reader.skipBits(8);  // vps_ols_ptl_idx[i]

        if (!vps_each_layer_is_an_ols_flag)
        {
            unsigned NumMultiLayerOlss = 0;
            int NumLayersInOls = 0;
            for (int i = 1; i < TotalNumOlss; i++)
            {
                if (vps_each_layer_is_an_ols_flag)
                    NumLayersInOls = 1;
                else if (vps_ols_mode_idc == 0 || vps_ols_mode_idc == 1)
                    NumLayersInOls = i + 1;
                else if (vps_ols_mode_idc == 2)
                {
                    int j = 0;
                    for (int k = 0; k < vps_max_layers; k++) NumLayersInOls = j;
                }
                if (NumLayersInOls > 1)
                    NumMultiLayerOlss++;
            }

            unsigned vps_num_dpb_params = extractUEGolombCode() + 1;
            if (vps_num_dpb_params >= NumMultiLayerOlss)
                return 1;
            unsigned VpsNumDpbParams;
            if (vps_each_layer_is_an_ols_flag)
                VpsNumDpbParams = 0;
            else
                VpsNumDpbParams = vps_num_dpb_params;

            bool vps_sublayer_dpb_params_present_flag =
                (vps_max_sublayers > 1) ? vps_sublayer_dpb_params_present_flag = m_reader.getBit() : 0;

            for (size_t i = 0; i < VpsNumDpbParams; i++)
            {
                int vps_dpb_max_tid = vps_max_sublayers - 1;
                if (!vps_default_ptl_dpb_hrd_max_tid_flag)
                    vps_dpb_max_tid = m_reader.getBits(3);
                if (dpb_parameters(vps_dpb_max_tid, vps_sublayer_dpb_params_present_flag))
                    return 1;
            }

            for (size_t i = 0; i < NumMultiLayerOlss; i++)
            {
                extractUEGolombCode();  // vps_ols_dpb_pic_width
                extractUEGolombCode();  // vps_ols_dpb_pic_height
                m_reader.skipBits(2);   // vps_ols_dpb_chroma_format
                unsigned vps_ols_dpb_bitdepth_minus8 = extractUEGolombCode();
                if (vps_ols_dpb_bitdepth_minus8 > 2)
                    return 1;
                if (VpsNumDpbParams > 1 && VpsNumDpbParams != NumMultiLayerOlss)
                {
                    unsigned vps_ols_dpb_params_idx = extractUEGolombCode();
                    if (vps_ols_dpb_params_idx >= VpsNumDpbParams)
                        return 1;
                }
            }
            bool vps_timing_hrd_params_present_flag = m_reader.getBit();
            if (vps_timing_hrd_params_present_flag)
            {
                if (m_vps_hrd.general_timing_hrd_parameters())
                    return 1;
                bool vps_sublayer_cpb_params_present_flag = (vps_max_sublayers > 1) ? m_reader.getBit() : 0;
                unsigned vps_num_ols_timing_hrd_params = extractUEGolombCode() + 1;
                if (vps_num_ols_timing_hrd_params > NumMultiLayerOlss)
                    return 1;
                for (size_t i = 0; i <= vps_num_ols_timing_hrd_params; i++)
                {
                    int vps_hrd_max_tid = 1;
                    if (!vps_default_ptl_dpb_hrd_max_tid_flag)
                        vps_hrd_max_tid = m_reader.getBits(3);
                    int firstSubLayer = vps_sublayer_cpb_params_present_flag ? 0 : vps_hrd_max_tid;
                    m_vps_hrd.ols_timing_hrd_parameters(firstSubLayer, vps_hrd_max_tid);
                }
                if (vps_num_ols_timing_hrd_params > 1 && vps_num_ols_timing_hrd_params != NumMultiLayerOlss)
                {
                    for (size_t i = 0; i < NumMultiLayerOlss; i++)
                    {
                        unsigned vps_ols_timing_hrd_idx = extractUEGolombCode();
                        if (vps_ols_timing_hrd_idx >= vps_num_ols_timing_hrd_params)
                            return 1;
                    }
                }
            }
        }

        return rez;
    }
    catch (VodCoreException& e)
    {
        return NOT_ENOUGH_BUFFER;
    }
}