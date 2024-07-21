de265_error seq_parameter_set::read(error_queue* errqueue, bitreader* br)
{
  int vlc;

  video_parameter_set_id = get_bits(br,4);
  sps_max_sub_layers     = get_bits(br,3) +1;
  if (sps_max_sub_layers>7) {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  sps_temporal_id_nesting_flag = get_bits(br,1);

  profile_tier_level_.read(br, sps_max_sub_layers);

  READ_VLC(seq_parameter_set_id, uvlc);
  if (seq_parameter_set_id >= DE265_MAX_SPS_SETS) {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }


  // --- decode chroma type ---

  READ_VLC(chroma_format_idc, uvlc);

  if (chroma_format_idc == 3) {
    separate_colour_plane_flag = get_bits(br,1);
  }
  else {
    separate_colour_plane_flag = 0;
  }

  if (chroma_format_idc<0 ||
      chroma_format_idc>3) {
    errqueue->add_warning(DE265_WARNING_INVALID_CHROMA_FORMAT, false);
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }


  // --- picture size ---

  READ_VLC(pic_width_in_luma_samples,  uvlc);
  READ_VLC(pic_height_in_luma_samples, uvlc);

  if (pic_width_in_luma_samples  == 0 ||
      pic_height_in_luma_samples == 0) {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  if (pic_width_in_luma_samples > MAX_PICTURE_WIDTH ||
      pic_height_in_luma_samples> MAX_PICTURE_HEIGHT) {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  conformance_window_flag = get_bits(br,1);

  if (conformance_window_flag) {
    READ_VLC(conf_win_left_offset,  uvlc);
    READ_VLC(conf_win_right_offset, uvlc);
    READ_VLC(conf_win_top_offset,   uvlc);
    READ_VLC(conf_win_bottom_offset,uvlc);
  }
  else {
    conf_win_left_offset  = 0;
    conf_win_right_offset = 0;
    conf_win_top_offset   = 0;
    conf_win_bottom_offset= 0;
  }

  READ_VLC_OFFSET(bit_depth_luma,  uvlc, 8);
  READ_VLC_OFFSET(bit_depth_chroma,uvlc, 8);
  if (bit_depth_luma > 16 ||
      bit_depth_chroma > 16) {
    errqueue->add_warning(DE265_WARNING_SPS_HEADER_INVALID, false);
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  READ_VLC_OFFSET(log2_max_pic_order_cnt_lsb, uvlc, 4);
  if (log2_max_pic_order_cnt_lsb<4 ||
      log2_max_pic_order_cnt_lsb>16) {
    errqueue->add_warning(DE265_WARNING_SPS_HEADER_INVALID, false);
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }
  MaxPicOrderCntLsb = 1<<(log2_max_pic_order_cnt_lsb);


  // --- sub_layer_ordering_info ---

  sps_sub_layer_ordering_info_present_flag = get_bits(br,1);

  int firstLayer = (sps_sub_layer_ordering_info_present_flag ?
                    0 : sps_max_sub_layers-1 );

  for (int i=firstLayer ; i <= sps_max_sub_layers-1; i++ ) {

    // sps_max_dec_pic_buffering[i]

    vlc=get_uvlc(br);
    if (vlc == UVLC_ERROR ||
        vlc+1 > MAX_NUM_REF_PICS) {
      errqueue->add_warning(DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }

    sps_max_dec_pic_buffering[i] = vlc+1;

    // sps_max_num_reorder_pics[i]

    READ_VLC(sps_max_num_reorder_pics[i], uvlc);


    // sps_max_latency_increase[i]

    READ_VLC(sps_max_latency_increase_plus1[i], uvlc);

    SpsMaxLatencyPictures[i] = (sps_max_num_reorder_pics[i] +
                                sps_max_latency_increase_plus1[i]-1);
  }

  // copy info to all layers if only specified once

  if (sps_sub_layer_ordering_info_present_flag) {
    int ref = sps_max_sub_layers-1;
    assert(ref<7);

    for (int i=0 ; i < sps_max_sub_layers-1; i++ ) {
      sps_max_dec_pic_buffering[i] = sps_max_dec_pic_buffering[ref];
      sps_max_num_reorder_pics[i]  = sps_max_num_reorder_pics[ref];
      sps_max_latency_increase_plus1[i]  = sps_max_latency_increase_plus1[ref];
    }
  }


  READ_VLC_OFFSET(log2_min_luma_coding_block_size, uvlc, 3);
  READ_VLC       (log2_diff_max_min_luma_coding_block_size, uvlc);
  READ_VLC_OFFSET(log2_min_transform_block_size, uvlc, 2);
  READ_VLC(log2_diff_max_min_transform_block_size, uvlc);
  READ_VLC(max_transform_hierarchy_depth_inter, uvlc);
  READ_VLC(max_transform_hierarchy_depth_intra, uvlc);

  if (log2_min_luma_coding_block_size > 6) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }
  if (log2_min_luma_coding_block_size + log2_diff_max_min_luma_coding_block_size > 6) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }
  if (log2_min_transform_block_size > 5) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }
  if (log2_min_transform_block_size + log2_diff_max_min_transform_block_size > 5) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }

  scaling_list_enable_flag = get_bits(br,1);

  if (scaling_list_enable_flag) {

    sps_scaling_list_data_present_flag = get_bits(br,1);
    if (sps_scaling_list_data_present_flag) {

      de265_error err;
      if ((err=read_scaling_list(br,this, &scaling_list, false)) != DE265_OK) {
        return err;
      }
    }
    else {
      set_default_scaling_lists(&scaling_list);
    }
  }

  amp_enabled_flag = get_bits(br,1);
  sample_adaptive_offset_enabled_flag = get_bits(br,1);
  pcm_enabled_flag = get_bits(br,1);
  if (pcm_enabled_flag) {
    pcm_sample_bit_depth_luma = get_bits(br,4)+1;
    pcm_sample_bit_depth_chroma = get_bits(br,4)+1;
    READ_VLC_OFFSET(log2_min_pcm_luma_coding_block_size, uvlc, 3);
    READ_VLC(log2_diff_max_min_pcm_luma_coding_block_size, uvlc);
    pcm_loop_filter_disable_flag = get_bits(br,1);

    if (pcm_sample_bit_depth_luma > bit_depth_luma) {
      errqueue->add_warning(DE265_WARNING_PCM_BITDEPTH_TOO_LARGE, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }

    if (pcm_sample_bit_depth_chroma > bit_depth_chroma) {
      errqueue->add_warning(DE265_WARNING_PCM_BITDEPTH_TOO_LARGE, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }
  }
  else {
    pcm_sample_bit_depth_luma = 0;
    pcm_sample_bit_depth_chroma = 0;
    log2_min_pcm_luma_coding_block_size = 0;
    log2_diff_max_min_pcm_luma_coding_block_size = 0;
    pcm_loop_filter_disable_flag = 0;
  }

  int num_short_term_ref_pic_sets;
  READ_VLC(num_short_term_ref_pic_sets, uvlc);
  if (num_short_term_ref_pic_sets < 0 ||
      num_short_term_ref_pic_sets > 64) {
    errqueue->add_warning(DE265_WARNING_NUMBER_OF_SHORT_TERM_REF_PIC_SETS_OUT_OF_RANGE, false);
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  // --- allocate reference pic set ---

  // we do not allocate the ref-pic-set for the slice header here, but in the slice header itself

  ref_pic_sets.resize(num_short_term_ref_pic_sets);

  for (int i = 0; i < num_short_term_ref_pic_sets; i++) {

    bool success = read_short_term_ref_pic_set(errqueue,this,br,
                                               &ref_pic_sets[i], i,
                                               ref_pic_sets,
                                               false);

    if (!success) {
      return DE265_WARNING_SPS_HEADER_INVALID;
    }

    // dump_short_term_ref_pic_set(&(*ref_pic_sets)[i], fh);
  }

  long_term_ref_pics_present_flag = get_bits(br,1);

  if (long_term_ref_pics_present_flag) {

    READ_VLC(num_long_term_ref_pics_sps, uvlc);
    if (num_long_term_ref_pics_sps > MAX_NUM_LT_REF_PICS_SPS) {
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }

    for (int i = 0; i < num_long_term_ref_pics_sps; i++ ) {
      lt_ref_pic_poc_lsb_sps[i] = get_bits(br, log2_max_pic_order_cnt_lsb);
      used_by_curr_pic_lt_sps_flag[i] = get_bits(br,1);
    }
  }
  else {
    num_long_term_ref_pics_sps = 0; // NOTE: missing definition in standard !
  }

  sps_temporal_mvp_enabled_flag = get_bits(br,1);
  strong_intra_smoothing_enable_flag = get_bits(br,1);

  vui_parameters_present_flag = get_bits(br,1);
  if (vui_parameters_present_flag) {
    vui.read(errqueue, br, this);
  }


  sps_extension_present_flag = get_bits(br,1);
  if (sps_extension_present_flag) {
    sps_range_extension_flag = get_bits(br,1);
    sps_multilayer_extension_flag = get_bits(br,1);
    sps_extension_6bits = get_bits(br,6);
  }
  else {
    sps_range_extension_flag = 0;
  }

  if (sps_range_extension_flag) {
    de265_error err = range_extension.read(errqueue, br);
    if (err != DE265_OK) { return err; }
  }

  /*
  sps_extension_flag = get_bits(br,1);
  if (sps_extension_flag) {
    assert(false);
  }
  */


  de265_error err = compute_derived_values();
  if (err != DE265_OK) { return err; }

  sps_read = true;

  return DE265_OK;
}