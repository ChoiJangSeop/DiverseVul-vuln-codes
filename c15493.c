void HevcUnitWithProfile::profile_tier_level(int subLayers)
{
    int profile_space = m_reader.getBits(2);
    int tier_flag = m_reader.getBit();
    profile_idc = m_reader.getBits(5);
    m_reader.skipBits(32);  // general_profile_compatibility_flag
    int progressive_source_flag = m_reader.getBit();
    interlaced_source_flag = m_reader.getBit();
    int non_packed_constraint_flag = m_reader.getBit();
    int frame_only_constraint_flag = m_reader.getBit();
    m_reader.skipBits(32);  // reserved_zero_44bits
    m_reader.skipBits(12);  // reserved_zero_44bits
    level_idc = m_reader.getBits(8);

    for (int i = 0; i < subLayers - 1; i++)
    {
        sub_layer_profile_present_flag[i] = m_reader.getBit();
        sub_layer_level_present_flag[i] = m_reader.getBit();
    }
    if (subLayers > 1)
    {
        for (int i = subLayers - 1; i < 8; i++) m_reader.skipBits(2);  // reserved_zero_2bits
    }

    for (int i = 0; i < subLayers - 1; i++)
    {
        if (sub_layer_profile_present_flag[i])
        {
            sub_layer_profile_space[i] = m_reader.getBits(2);
            bool sub_layer_tier_flag = m_reader.getBit();
            int sub_layer_profile_idc = m_reader.getBits(5);
            for (int j = 0; j < 32; j++) m_reader.skipBit();  // sub_layer_profile_compatibility_flag[ i ][ j ] u(1)
            m_reader.skipBit();                               // sub_layer_progressive_source_flag[ i ] u(1)
            m_reader.skipBit();                               // sub_layer_interlaced_source_flag[ i ] u(1)
            m_reader.skipBit();                               // sub_layer_non_packed_constraint_flag[ i ] u(1)
            m_reader.skipBit();                               // sub_layer_frame_only_constraint_flag[ i ] u(1)
            m_reader.skipBits(32);                            // sub_layer_reserved_zero_44bits
            m_reader.skipBits(12);                            // sub_layer_reserved_zero_44bits
        }
        if (sub_layer_level_present_flag[i])
            m_reader.skipBits(8);  // sub_layer_level_idc[ i ]
    }
}