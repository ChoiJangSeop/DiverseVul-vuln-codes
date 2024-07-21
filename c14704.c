de265_error slice_segment_header::read(bitreader* br, decoder_context* ctx,
                                       bool* continueDecoding)
{
  *continueDecoding = false;
  reset();

  // set defaults

  dependent_slice_segment_flag = 0;


  // read bitstream

  first_slice_segment_in_pic_flag = get_bits(br,1);

  if (ctx->get_RapPicFlag()) { // TODO: is this still correct ? Should we drop RapPicFlag ?
    no_output_of_prior_pics_flag = get_bits(br,1);
  }

  slice_pic_parameter_set_id = get_uvlc(br);
  if (slice_pic_parameter_set_id > DE265_MAX_PPS_SETS ||
      slice_pic_parameter_set_id == UVLC_ERROR) {
    ctx->add_warning(DE265_WARNING_NONEXISTING_PPS_REFERENCED, false);
    return DE265_OK;
  }

  if (!ctx->has_pps(slice_pic_parameter_set_id)) {
    ctx->add_warning(DE265_WARNING_NONEXISTING_PPS_REFERENCED, false);
    return DE265_OK;
  }

  pps = ctx->get_shared_pps(slice_pic_parameter_set_id);

  const seq_parameter_set* sps = pps->sps.get();
  if (!sps->sps_read) {
    ctx->add_warning(DE265_WARNING_NONEXISTING_SPS_REFERENCED, false);
    *continueDecoding = false;
    return DE265_OK;
  }

  if (!first_slice_segment_in_pic_flag) {
    if (pps->dependent_slice_segments_enabled_flag) {
      dependent_slice_segment_flag = get_bits(br,1);
    } else {
      dependent_slice_segment_flag = 0;
    }

    int slice_segment_address = get_bits(br, ceil_log2(sps->PicSizeInCtbsY));

    if (dependent_slice_segment_flag) {
      if (slice_segment_address == 0) {
        *continueDecoding = false;
        ctx->add_warning(DE265_WARNING_DEPENDENT_SLICE_WITH_ADDRESS_ZERO, false);
        return DE265_OK;
      }

      if (ctx->previous_slice_header == NULL) {
        return DE265_ERROR_NO_INITIAL_SLICE_HEADER;
      }

      *this = *ctx->previous_slice_header;

      first_slice_segment_in_pic_flag = 0;
      dependent_slice_segment_flag = 1;
    }

    this->slice_segment_address = slice_segment_address;
  } else {
    dependent_slice_segment_flag = 0;
    slice_segment_address = 0;
  }

  if (slice_segment_address < 0 ||
      slice_segment_address >= sps->PicSizeInCtbsY) {
    ctx->add_warning(DE265_WARNING_SLICE_SEGMENT_ADDRESS_INVALID, false);
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  //printf("SLICE %d (%d)\n",slice_segment_address, sps->PicSizeInCtbsY);


  if (!dependent_slice_segment_flag) {
    for (int i=0; i<pps->num_extra_slice_header_bits; i++) {
      //slice_reserved_undetermined_flag[i]
      skip_bits(br,1);
    }

    slice_type = get_uvlc(br);
    if (slice_type > 2 ||
	slice_type == UVLC_ERROR) {
      ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
      *continueDecoding = false;
      return DE265_OK;
    }

    if (pps->output_flag_present_flag) {
      pic_output_flag = get_bits(br,1);
    }
    else {
      pic_output_flag = 1;
    }

    if (sps->separate_colour_plane_flag == 1) {
      colour_plane_id = get_bits(br,2);
    }


    slice_pic_order_cnt_lsb = 0;
    short_term_ref_pic_set_sps_flag = 0;

    int NumLtPics = 0;

    if (ctx->get_nal_unit_type() != NAL_UNIT_IDR_W_RADL &&
        ctx->get_nal_unit_type() != NAL_UNIT_IDR_N_LP) {
      slice_pic_order_cnt_lsb = get_bits(br, sps->log2_max_pic_order_cnt_lsb);
      short_term_ref_pic_set_sps_flag = get_bits(br,1);

      if (!short_term_ref_pic_set_sps_flag) {
        read_short_term_ref_pic_set(ctx, sps,
                                    br, &slice_ref_pic_set,
                                    sps->num_short_term_ref_pic_sets(),
                                    sps->ref_pic_sets,
                                    true);

        CurrRpsIdx = sps->num_short_term_ref_pic_sets();
        CurrRps    = slice_ref_pic_set;
      }
      else {
        int nBits = ceil_log2(sps->num_short_term_ref_pic_sets());
        if (nBits>0) short_term_ref_pic_set_idx = get_bits(br,nBits);
        else         short_term_ref_pic_set_idx = 0;

        if (short_term_ref_pic_set_idx >= sps->num_short_term_ref_pic_sets()) {
          ctx->add_warning(DE265_WARNING_SHORT_TERM_REF_PIC_SET_OUT_OF_RANGE, false);
          return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
        }

        CurrRpsIdx = short_term_ref_pic_set_idx;
        CurrRps    = sps->ref_pic_sets[CurrRpsIdx];
      }


      // --- long-term MC ---

      if (sps->long_term_ref_pics_present_flag) {
        if (sps->num_long_term_ref_pics_sps > 0) {
          num_long_term_sps = get_uvlc(br);
          if (num_long_term_sps == UVLC_ERROR) {
            return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
          }
        }
        else {
          num_long_term_sps = 0;
        }

        num_long_term_pics= get_uvlc(br);
        if (num_long_term_pics == UVLC_ERROR) {
          return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
        }

        // check maximum number of reference frames

        if (num_long_term_sps +
            num_long_term_pics +
            CurrRps.NumNegativePics +
            CurrRps.NumPositivePics
            > sps->sps_max_dec_pic_buffering[sps->sps_max_sub_layers-1])
          {
            ctx->add_warning(DE265_WARNING_MAX_NUM_REF_PICS_EXCEEDED, false);
            *continueDecoding = false;
            return DE265_OK;
          }

        for (int i=0; i<num_long_term_sps + num_long_term_pics; i++) {
          if (i < num_long_term_sps) {
            int nBits = ceil_log2(sps->num_long_term_ref_pics_sps);
            lt_idx_sps[i] = get_bits(br, nBits);

            // check that the referenced lt-reference really exists

            if (lt_idx_sps[i] >= sps->num_long_term_ref_pics_sps) {
              ctx->add_warning(DE265_NON_EXISTING_LT_REFERENCE_CANDIDATE_IN_SLICE_HEADER, false);
              *continueDecoding = false;
              return DE265_OK;
            }

            // delta_poc_msb_present_flag[i] = 0; // TODO ?

            ctx->PocLsbLt[i] = sps->lt_ref_pic_poc_lsb_sps[ lt_idx_sps[i] ];
            ctx->UsedByCurrPicLt[i] = sps->used_by_curr_pic_lt_sps_flag[ lt_idx_sps[i] ];
          }
          else {
            int nBits = sps->log2_max_pic_order_cnt_lsb;
            poc_lsb_lt[i] = get_bits(br, nBits);
            used_by_curr_pic_lt_flag[i] = get_bits(br,1);

            ctx->PocLsbLt[i] = poc_lsb_lt[i];
            ctx->UsedByCurrPicLt[i] = used_by_curr_pic_lt_flag[i];
          }

          if (ctx->UsedByCurrPicLt[i]) {
            NumLtPics++;
          }

          delta_poc_msb_present_flag[i] = get_bits(br,1);
          if (delta_poc_msb_present_flag[i]) {
            delta_poc_msb_cycle_lt[i] = get_uvlc(br);
            if (delta_poc_msb_cycle_lt[i]==UVLC_ERROR) {
              return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
            }
          }
          else {
            delta_poc_msb_cycle_lt[i] = 0;
          }

          if (i==0 || i==num_long_term_sps) {
            ctx->DeltaPocMsbCycleLt[i] = delta_poc_msb_cycle_lt[i];
          }
          else {
            ctx->DeltaPocMsbCycleLt[i] = (delta_poc_msb_cycle_lt[i] +
                                          ctx->DeltaPocMsbCycleLt[i-1]);
          }
        }
      }
      else {
        num_long_term_sps = 0;
        num_long_term_pics= 0;
      }

      if (sps->sps_temporal_mvp_enabled_flag) {
        slice_temporal_mvp_enabled_flag = get_bits(br,1);
      }
      else {
        slice_temporal_mvp_enabled_flag = 0;
      }
    }
    else {
      slice_pic_order_cnt_lsb = 0;
      num_long_term_sps = 0;
      num_long_term_pics= 0;
    }


    // --- SAO ---

    if (sps->sample_adaptive_offset_enabled_flag) {
      slice_sao_luma_flag   = get_bits(br,1);

      if (sps->ChromaArrayType != CHROMA_MONO) {
        slice_sao_chroma_flag = get_bits(br,1);
      }
      else {
        slice_sao_chroma_flag = 0;
      }
    }
    else {
      slice_sao_luma_flag   = 0;
      slice_sao_chroma_flag = 0;
    }

    num_ref_idx_l0_active = 0;
    num_ref_idx_l1_active = 0;

    if (slice_type == SLICE_TYPE_P  ||
        slice_type == SLICE_TYPE_B) {
      num_ref_idx_active_override_flag = get_bits(br,1);
      if (num_ref_idx_active_override_flag) {
        num_ref_idx_l0_active = get_uvlc(br);
        if (num_ref_idx_l0_active == UVLC_ERROR) {
	  ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
          return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
	}
        num_ref_idx_l0_active++;;

        if (slice_type == SLICE_TYPE_B) {
          num_ref_idx_l1_active = get_uvlc(br);
          if (num_ref_idx_l1_active == UVLC_ERROR) {
	    ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
	  }
          num_ref_idx_l1_active++;
        }
      }
      else {
        num_ref_idx_l0_active = pps->num_ref_idx_l0_default_active;
        num_ref_idx_l1_active = pps->num_ref_idx_l1_default_active;
      }

      if (num_ref_idx_l0_active > 16) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }
      if (num_ref_idx_l1_active > 16) { return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE; }

      NumPocTotalCurr = CurrRps.NumPocTotalCurr_shortterm_only + NumLtPics;

      if (pps->lists_modification_present_flag && NumPocTotalCurr > 1) {

        int nBits = ceil_log2(NumPocTotalCurr);

        ref_pic_list_modification_flag_l0 = get_bits(br,1);
        if (ref_pic_list_modification_flag_l0) {
          for (int i=0;i<num_ref_idx_l0_active;i++) {
            list_entry_l0[i] = get_bits(br, nBits);
          }
        }

        if (slice_type == SLICE_TYPE_B) {
          ref_pic_list_modification_flag_l1 = get_bits(br,1);
          if (ref_pic_list_modification_flag_l1) {
            for (int i=0;i<num_ref_idx_l1_active;i++) {
              list_entry_l1[i] = get_bits(br, nBits);
            }
          }
        }
        else {
          ref_pic_list_modification_flag_l1 = 0;
        }
      }
      else {
        ref_pic_list_modification_flag_l0 = 0;
        ref_pic_list_modification_flag_l1 = 0;
      }

      if (slice_type == SLICE_TYPE_B) {
        mvd_l1_zero_flag = get_bits(br,1);
      }

      if (pps->cabac_init_present_flag) {
        cabac_init_flag = get_bits(br,1);
      }
      else {
        cabac_init_flag = 0;
      }

      if (slice_temporal_mvp_enabled_flag) {
        if (slice_type == SLICE_TYPE_B)
          collocated_from_l0_flag = get_bits(br,1);
        else
          collocated_from_l0_flag = 1;

        if (( collocated_from_l0_flag && num_ref_idx_l0_active > 1) ||
            (!collocated_from_l0_flag && num_ref_idx_l1_active > 1)) {
          collocated_ref_idx = get_uvlc(br);
          if (collocated_ref_idx == UVLC_ERROR) {
	    ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
	  }
        }
        else {
          collocated_ref_idx = 0;
        }

        // check whether collocated_ref_idx points to a valid index

        if (( collocated_from_l0_flag && collocated_ref_idx >= num_ref_idx_l0_active) ||
            (!collocated_from_l0_flag && collocated_ref_idx >= num_ref_idx_l1_active)) {
          ctx->add_warning(DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE, false);
          return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
        }
      }


      if ((pps->weighted_pred_flag   && slice_type == SLICE_TYPE_P) ||
          (pps->weighted_bipred_flag && slice_type == SLICE_TYPE_B)) {

        if (!read_pred_weight_table(br,this,ctx))
          {
	    ctx->add_warning(DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE, false);
	    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
          }
      }

      five_minus_max_num_merge_cand = get_uvlc(br);
      if (five_minus_max_num_merge_cand == UVLC_ERROR) {
	ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }
      MaxNumMergeCand = 5-five_minus_max_num_merge_cand;
    }

    slice_qp_delta = get_svlc(br);
    if (slice_qp_delta == UVLC_ERROR) {
      ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }
    //logtrace(LogSlice,"slice_qp_delta: %d\n",shdr->slice_qp_delta);

    if (pps->pps_slice_chroma_qp_offsets_present_flag) {
      slice_cb_qp_offset = get_svlc(br);
      if (slice_cb_qp_offset == UVLC_ERROR) {
	ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }

      slice_cr_qp_offset = get_svlc(br);
      if (slice_cr_qp_offset == UVLC_ERROR) {
	ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }
    }
    else {
      slice_cb_qp_offset = 0;
      slice_cr_qp_offset = 0;
    }

    if (pps->range_extension.chroma_qp_offset_list_enabled_flag) {
      cu_chroma_qp_offset_enabled_flag = get_bits(br,1);
    }

    if (pps->deblocking_filter_override_enabled_flag) {
      deblocking_filter_override_flag = get_bits(br,1);
    }
    else {
      deblocking_filter_override_flag = 0;
    }

    slice_beta_offset = pps->beta_offset;
    slice_tc_offset   = pps->tc_offset;

    if (deblocking_filter_override_flag) {
      slice_deblocking_filter_disabled_flag = get_bits(br,1);
      if (!slice_deblocking_filter_disabled_flag) {
        slice_beta_offset = get_svlc(br);
        if (slice_beta_offset == UVLC_ERROR) {
	  ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	  return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
	}
        slice_beta_offset *= 2;

        slice_tc_offset   = get_svlc(br);
        if (slice_tc_offset == UVLC_ERROR) {
	  ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	  return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
	}
        slice_tc_offset   *= 2;
      }
    }
    else {
      slice_deblocking_filter_disabled_flag = pps->pic_disable_deblocking_filter_flag;
    }

    if (pps->pps_loop_filter_across_slices_enabled_flag  &&
        (slice_sao_luma_flag || slice_sao_chroma_flag ||
         !slice_deblocking_filter_disabled_flag )) {
      slice_loop_filter_across_slices_enabled_flag = get_bits(br,1);
    }
    else {
      slice_loop_filter_across_slices_enabled_flag =
        pps->pps_loop_filter_across_slices_enabled_flag;
    }
  }

  if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag ) {
    num_entry_point_offsets = get_uvlc(br);
    if (num_entry_point_offsets == UVLC_ERROR) {
      ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }

    if (pps->entropy_coding_sync_enabled_flag) {
      // check num_entry_points for valid range

      int firstCTBRow = slice_segment_address / sps->PicWidthInCtbsY;
      int lastCTBRow  = firstCTBRow + num_entry_point_offsets;
      if (lastCTBRow >= sps->PicHeightInCtbsY) {
        ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
        return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }
    }

    if (pps->tiles_enabled_flag) {
      if (num_entry_point_offsets > pps->num_tile_columns * pps->num_tile_rows) {
        ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
        return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }
    }

    entry_point_offset.resize( num_entry_point_offsets );

    if (num_entry_point_offsets > 0) {
      offset_len = get_uvlc(br);
      if (offset_len == UVLC_ERROR) {
	ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
	return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }
      offset_len++;

      if (offset_len > 32) {
	return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
      }

      for (int i=0; i<num_entry_point_offsets; i++) {
        {
          entry_point_offset[i] = get_bits(br,offset_len)+1;
        }

        if (i>0) {
          entry_point_offset[i] += entry_point_offset[i-1];
        }
      }
    }
  }
  else {
    num_entry_point_offsets = 0;
  }

  if (pps->slice_segment_header_extension_present_flag) {
    slice_segment_header_extension_length = get_uvlc(br);
    if (slice_segment_header_extension_length == UVLC_ERROR ||
	slice_segment_header_extension_length > 1000) {  // TODO: safety check against too large values
      ctx->add_warning(DE265_WARNING_SLICEHEADER_INVALID, false);
      return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
    }

    for (int i=0; i<slice_segment_header_extension_length; i++) {
      //slice_segment_header_extension_data_byte[i]
      get_bits(br,8);
    }
  }


  compute_derived_values(pps.get());

  *continueDecoding = true;
  return DE265_OK;
}