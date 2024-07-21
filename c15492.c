int HevcVpsUnit::deserialize()
{
    int rez = HevcUnit::deserialize();
    if (rez)
        return rez;

    if (m_reader.getBitsLeft() < 128)
        return NOT_ENOUGH_BUFFER;

    try
    {
        vps_id = m_reader.getBits(4);
        m_reader.skipBits(2);  // reserved
        vps_max_layers = m_reader.getBits(6) + 1;
        vps_max_sub_layers = m_reader.getBits(3) + 1;
        int vps_temporal_id_nesting_flag = m_reader.getBit();
        m_reader.skipBits(16);  // reserved
        profile_tier_level(vps_max_sub_layers);

        bool vps_sub_layer_ordering_info_present_flag = m_reader.getBit();
        for (int i = (vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers - 1);
             i <= vps_max_sub_layers - 1; i++)
        {
            unsigned vps_max_dec_pic_buffering_minus1 = extractUEGolombCode();
            unsigned vps_max_num_reorder_pics = extractUEGolombCode();
            if (vps_max_num_reorder_pics > vps_max_dec_pic_buffering_minus1)
                return 1;
            unsigned vps_max_latency_increase_plus1 = extractUEGolombCode();
            if (vps_max_latency_increase_plus1 == 0xffffffff)
                return 1;
        }
        int vps_max_layer_id = m_reader.getBits(6);
        unsigned vps_num_layer_sets_minus1 = extractUEGolombCode();
        if (vps_num_layer_sets_minus1 > 1023)
            return 1;
        for (size_t i = 1; i <= vps_num_layer_sets_minus1; i++)
        {
            for (int j = 0; j <= vps_max_layer_id; j++) m_reader.skipBit();  // layer_id_included_flag[ i ][ j ] u(1)
        }
        bool vps_timing_info_present_flag = m_reader.getBit();
        if (vps_timing_info_present_flag)
        {
            num_units_in_tick_bit_pos = m_reader.getBitsCount();
            num_units_in_tick = m_reader.getBits(32);
            time_scale = m_reader.getBits(32);
        }

        return rez;
    }
    catch (VodCoreException& e)
    {
        return NOT_ENOUGH_BUFFER;
    }
}