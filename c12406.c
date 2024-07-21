de265_error video_usability_information::hrd_parameters(error_queue* errqueue, bitreader* br, const seq_parameter_set* sps)
{
  int vlc;

  nal_hrd_parameters_present_flag = get_bits(br, 1);
  vcl_hrd_parameters_present_flag = get_bits(br, 1);

  if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
  {
    sub_pic_hrd_params_present_flag = get_bits(br, 1);
    if (sub_pic_hrd_params_present_flag)
    {
      tick_divisor_minus2 = get_bits(br, 8);
      du_cpb_removal_delay_increment_length_minus1 = get_bits(br, 5);
      sub_pic_cpb_params_in_pic_timing_sei_flag = get_bits(br, 1);
      dpb_output_delay_du_length_minus1 = get_bits(br, 5);
    }
    bit_rate_scale = get_bits(br, 4);
    cpb_size_scale = get_bits(br, 4);


    if (sub_pic_hrd_params_present_flag)
    {
      cpb_size_du_scale = get_bits(br, 4);
    }
    initial_cpb_removal_delay_length_minus1 = get_bits(br, 5);
    au_cpb_removal_delay_length_minus1 = get_bits(br, 5);
    dpb_output_delay_length_minus1 = get_bits(br, 5);
  }
  int  i, j, nalOrVcl;

  for (i = 0; i < sps->sps_max_sub_layers; i++)
  {
    fixed_pic_rate_general_flag[i] = get_bits(br, 1);
    if (!fixed_pic_rate_general_flag[i])
    {
      fixed_pic_rate_within_cvs_flag[i] = get_bits(br, 1);
    }
    else
    {
      fixed_pic_rate_within_cvs_flag[i] = true;
    }

    low_delay_hrd_flag[i] = 0;// Infered to be 0 when not present
    cpb_cnt_minus1[i] = 0;    // Infered to be 0 when not present

    if (fixed_pic_rate_within_cvs_flag[i])
    {
      READ_VLC_OFFSET(elemental_duration_in_tc_minus1[i], uvlc, 0);
    }
    else
    {
      low_delay_hrd_flag[i] = get_bits(br, 1);
    }
    if (!low_delay_hrd_flag[i])
    {
      READ_VLC_OFFSET(cpb_cnt_minus1[i], uvlc, 0);
    }

    for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
    {
      if (((nalOrVcl == 0) && nal_hrd_parameters_present_flag) ||
        ((nalOrVcl == 1) && vcl_hrd_parameters_present_flag))
      {
        for (j = 0; j <= cpb_cnt_minus1[i]; j++)
        {
          READ_VLC_OFFSET(bit_rate_value_minus1[i][j][nalOrVcl], uvlc, 0);
          READ_VLC_OFFSET(cpb_size_value_minus1[i][j][nalOrVcl], uvlc, 0);

          if (sub_pic_hrd_params_present_flag)
          {
            READ_VLC_OFFSET(cpb_size_du_value_minus1[i][j][nalOrVcl], uvlc, 0);
            READ_VLC_OFFSET(bit_rate_du_value_minus1[i][j][nalOrVcl], uvlc, 0);
          }
          cbr_flag[i][j][nalOrVcl] = get_bits(br, 1);
        }
      }
    }
  }
  return DE265_OK;
}