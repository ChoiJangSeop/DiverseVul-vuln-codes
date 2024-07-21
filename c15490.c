void VvcUnitWithProfile::profile_tier_level(bool profileTierPresentFlag, int MaxNumSubLayersMinus1)
{
    if (profileTierPresentFlag)
    {
        profile_idc = m_reader.getBits(7);
        bool tier_flag = m_reader.getBit();
    }
    level_idc = m_reader.getBits(8);
    m_reader.skipBits(2);  // ptl_frame_only_constraint_flag, ptl_multilayer_enabled_flag

    if (profileTierPresentFlag)
    {                           // general_constraints_info()
        if (m_reader.getBit())  // gci_present_flag
        {
            m_reader.skipBits(32);
            m_reader.skipBits(32);
            m_reader.skipBits(7);
            int gci_num_reserved_bits = m_reader.getBits(8);
            for (int i = 0; i < gci_num_reserved_bits; i++) m_reader.skipBit();  // gci_reserved_zero_bit[i]
        }
        m_reader.skipBits(m_reader.getBitsLeft() % 8);  // gci_alignment_zero_bit
    }
    std::vector<int> ptl_sublayer_level_present_flag;
    ptl_sublayer_level_present_flag.resize(MaxNumSubLayersMinus1);

    for (int i = MaxNumSubLayersMinus1 - 1; i >= 0; i--) ptl_sublayer_level_present_flag[i] = m_reader.getBit();

    m_reader.skipBits(m_reader.getBitsLeft() % 8);  // ptl_reserved_zero_bit

    for (int i = MaxNumSubLayersMinus1 - 1; i >= 0; i--)
        if (ptl_sublayer_level_present_flag[i])
            m_reader.skipBits(8);  // sublayer_level_idc[i]
    if (profileTierPresentFlag)
    {
        int ptl_num_sub_profiles = m_reader.getBits(8);
        for (int i = 0; i < ptl_num_sub_profiles; i++) m_reader.skipBits(32);  // general_sub_profile_idc[i]
    }
}