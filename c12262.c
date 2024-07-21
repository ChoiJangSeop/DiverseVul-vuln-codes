s32 hevc_parse_slice_segment(GF_BitStream *bs, HEVCState *hevc, HEVCSliceInfo *si)
{
	u32 i, j;
	u32 num_ref_idx_l0_active = 0, num_ref_idx_l1_active = 0;
	HEVC_PPS *pps;
	HEVC_SPS *sps;
	s32 pps_id;
	Bool RapPicFlag = GF_FALSE;
	Bool IDRPicFlag = GF_FALSE;

	si->first_slice_segment_in_pic_flag = gf_bs_read_int_log(bs, 1, "first_slice_segment_in_pic_flag");

	switch (si->nal_unit_type) {
	case GF_HEVC_NALU_SLICE_IDR_W_DLP:
	case GF_HEVC_NALU_SLICE_IDR_N_LP:
		IDRPicFlag = GF_TRUE;
		RapPicFlag = GF_TRUE;
		break;
	case GF_HEVC_NALU_SLICE_BLA_W_LP:
	case GF_HEVC_NALU_SLICE_BLA_W_DLP:
	case GF_HEVC_NALU_SLICE_BLA_N_LP:
	case GF_HEVC_NALU_SLICE_CRA:
		RapPicFlag = GF_TRUE;
		break;
	}

	if (RapPicFlag) {
		gf_bs_read_int_log(bs, 1, "no_output_of_prior_pics_flag");
	}

	pps_id = gf_bs_read_ue_log(bs, "pps_id");
	if (pps_id >= 64)
		return -1;

	pps = &hevc->pps[pps_id];
	sps = &hevc->sps[pps->sps_id];
	si->sps = sps;
	si->pps = pps;

	if (!si->first_slice_segment_in_pic_flag && pps->dependent_slice_segments_enabled_flag) {
		si->dependent_slice_segment_flag = gf_bs_read_int_log(bs, 1, "dependent_slice_segment_flag");
	}
	else {
		si->dependent_slice_segment_flag = GF_FALSE;
	}

	if (!si->first_slice_segment_in_pic_flag) {
		si->slice_segment_address = gf_bs_read_int_log(bs, sps->bitsSliceSegmentAddress, "slice_segment_address");
	}
	else {
		si->slice_segment_address = 0;
	}

	if (!si->dependent_slice_segment_flag) {
		Bool deblocking_filter_override_flag = 0;
		Bool slice_temporal_mvp_enabled_flag = 0;
		Bool slice_sao_luma_flag = 0;
		Bool slice_sao_chroma_flag = 0;
		Bool slice_deblocking_filter_disabled_flag = 0;

		//"slice_reserved_undetermined_flag[]"
		gf_bs_read_int_log(bs, pps->num_extra_slice_header_bits, "slice_reserved_undetermined_flag");

		si->slice_type = gf_bs_read_ue_log(bs, "slice_type");

		if (pps->output_flag_present_flag)
			gf_bs_read_int_log(bs, 1, "pic_output_flag");

		if (sps->separate_colour_plane_flag == 1)
			gf_bs_read_int_log(bs, 2, "colour_plane_id");

		if (IDRPicFlag) {
			si->poc_lsb = 0;

			//if not asked to parse full header, abort since we know the poc
			if (!hevc->full_slice_header_parse) return 0;

		}
		else {
			si->poc_lsb = gf_bs_read_int_log(bs, sps->log2_max_pic_order_cnt_lsb, "poc_lsb");

			//if not asked to parse full header, abort once we have the poc
			if (!hevc->full_slice_header_parse) return 0;

			if (gf_bs_read_int_log(bs, 1, "short_term_ref_pic_set_sps_flag") == 0) {
				Bool ret = hevc_parse_short_term_ref_pic_set(bs, sps, sps->num_short_term_ref_pic_sets);
				if (!ret)
					return -1;
			}
			else if (sps->num_short_term_ref_pic_sets > 1) {
				u32 numbits = 0;

				while ((u32)(1 << numbits) < sps->num_short_term_ref_pic_sets)
					numbits++;
				if (numbits > 0)
					gf_bs_read_int_log(bs, numbits, "short_term_ref_pic_set_idx");
				/*else
					short_term_ref_pic_set_idx = 0;*/
			}
			if (sps->long_term_ref_pics_present_flag) {
				u8 DeltaPocMsbCycleLt[32];
				u32 num_long_term_sps = 0;
				u32 num_long_term_pics = 0;

				memset(DeltaPocMsbCycleLt, 0, sizeof(u8) * 32);
				
				if (sps->num_long_term_ref_pic_sps > 0) {
					num_long_term_sps = gf_bs_read_ue_log(bs, "num_long_term_sps");
				}
				num_long_term_pics = gf_bs_read_ue_log(bs, "num_long_term_pics");

				for (i = 0; i < num_long_term_sps + num_long_term_pics; i++) {
					if (i < num_long_term_sps) {
						if (sps->num_long_term_ref_pic_sps > 1)
							gf_bs_read_int_log_idx(bs, gf_get_bit_size(sps->num_long_term_ref_pic_sps), "lt_idx_sps", i);
					}
					else {
						gf_bs_read_int_log_idx(bs, sps->log2_max_pic_order_cnt_lsb, "PocLsbLt", i);
						gf_bs_read_int_log_idx(bs, 1, "UsedByCurrPicLt", i);
					}
					if (gf_bs_read_int_log_idx(bs, 1, "delta_poc_msb_present_flag", i)) {
						if (i == 0 || i == num_long_term_sps)
							DeltaPocMsbCycleLt[i] = gf_bs_read_ue_log_idx(bs, "DeltaPocMsbCycleLt", i);
						else
							DeltaPocMsbCycleLt[i] = gf_bs_read_ue_log_idx(bs, "DeltaPocMsbCycleLt", i) + DeltaPocMsbCycleLt[i - 1];
					}
				}
			}
			if (sps->temporal_mvp_enable_flag)
				slice_temporal_mvp_enabled_flag = gf_bs_read_int_log(bs, 1, "slice_temporal_mvp_enabled_flag");
		}
		if (sps->sample_adaptive_offset_enabled_flag) {
			u32 ChromaArrayType = sps->separate_colour_plane_flag ? 0 : sps->chroma_format_idc;
			slice_sao_luma_flag = gf_bs_read_int_log(bs, 1, "slice_sao_luma_flag");
			if (ChromaArrayType != 0)
				slice_sao_chroma_flag = gf_bs_read_int_log(bs, 1, "slice_sao_chroma_flag");
		}

		if (si->slice_type == GF_HEVC_SLICE_TYPE_P || si->slice_type == GF_HEVC_SLICE_TYPE_B) {
			//u32 NumPocTotalCurr;
			num_ref_idx_l0_active = pps->num_ref_idx_l0_default_active;
			num_ref_idx_l1_active = 0;
			if (si->slice_type == GF_HEVC_SLICE_TYPE_B)
				num_ref_idx_l1_active = pps->num_ref_idx_l1_default_active;

			if (gf_bs_read_int_log(bs, 1, "num_ref_idx_active_override_flag")) {
				num_ref_idx_l0_active = 1 + gf_bs_read_ue_log(bs, "num_ref_idx_l0_active");
				if (si->slice_type == GF_HEVC_SLICE_TYPE_B)
					num_ref_idx_l1_active = 1 + gf_bs_read_ue_log(bs, "num_ref_idx_l1_active");
			}

			if (pps->lists_modification_present_flag /*TODO: && NumPicTotalCurr > 1*/) {
				if (!ref_pic_lists_modification(bs, si->slice_type, num_ref_idx_l0_active, num_ref_idx_l1_active)) {
					GF_LOG(GF_LOG_WARNING, GF_LOG_CODING, ("[hevc] ref_pic_lists_modification( ) not implemented\n"));
					return -1;
				}
			}

			if (si->slice_type == GF_HEVC_SLICE_TYPE_B)
				gf_bs_read_int_log(bs, 1, "mvd_l1_zero_flag");
			if (pps->cabac_init_present_flag)
				gf_bs_read_int_log(bs, 1, "cabac_init_flag");

			if (slice_temporal_mvp_enabled_flag) {
				// When collocated_from_l0_flag is not present, it is inferred to be equal to 1.
				Bool collocated_from_l0_flag = 1;
				if (si->slice_type == GF_HEVC_SLICE_TYPE_B)
					collocated_from_l0_flag = gf_bs_read_int_log(bs, 1, "collocated_from_l0_flag");

				if ((collocated_from_l0_flag && (num_ref_idx_l0_active > 1))
					|| (!collocated_from_l0_flag && (num_ref_idx_l1_active > 1))
				) {
					gf_bs_read_ue_log(bs, "collocated_ref_idx");
				}
			}

			if ((pps->weighted_pred_flag && si->slice_type == GF_HEVC_SLICE_TYPE_P)
				|| (pps->weighted_bipred_flag && si->slice_type == GF_HEVC_SLICE_TYPE_B)
				) {
				hevc_pred_weight_table(bs, hevc, si, pps, sps, num_ref_idx_l0_active, num_ref_idx_l1_active);
			}
			gf_bs_read_ue_log(bs, "five_minus_max_num_merge_cand");
		}
		si->slice_qp_delta_start_bits = (s32) (gf_bs_get_position(bs) - 1) * 8 + gf_bs_get_bit_position(bs);
		si->slice_qp_delta = gf_bs_read_se_log(bs, "slice_qp_delta");

		if (pps->slice_chroma_qp_offsets_present_flag) {
			gf_bs_read_se_log(bs, "slice_cb_qp_offset");
			gf_bs_read_se_log(bs, "slice_cr_qp_offset");
		}
		if (pps->deblocking_filter_override_enabled_flag) {
			deblocking_filter_override_flag = gf_bs_read_int_log(bs, 1, "deblocking_filter_override_flag");
		}

		if (deblocking_filter_override_flag) {
			slice_deblocking_filter_disabled_flag = gf_bs_read_int_log(bs, 1, "slice_deblocking_filter_disabled_flag");
			if (!slice_deblocking_filter_disabled_flag) {
				gf_bs_read_se_log(bs, "slice_beta_offset_div2");
				gf_bs_read_se_log(bs, "slice_tc_offset_div2");
			}
		}
		if (pps->loop_filter_across_slices_enabled_flag
			&& (slice_sao_luma_flag || slice_sao_chroma_flag || !slice_deblocking_filter_disabled_flag)
		) {
			gf_bs_read_int_log(bs, 1, "slice_loop_filter_across_slices_enabled_flag");
		}
	}
	//dependent slice segment
	else {
		//if not asked to parse full header, abort
		if (!hevc->full_slice_header_parse) return 0;
	}

	si->entry_point_start_bits = ((u32)gf_bs_get_position(bs) - 1) * 8 + gf_bs_get_bit_position(bs);

	if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag) {
		u32 num_entry_point_offsets = gf_bs_read_ue_log(bs, "num_entry_point_offsets");
		if (num_entry_point_offsets > 0) {
			u32 offset = gf_bs_read_ue_log(bs, "offset") + 1;
			u32 segments = offset >> 4;
			s32 remain = (offset & 15);

			for (i = 0; i < num_entry_point_offsets; i++) {
				//u32 res = 0;
				for (j = 0; j < segments; j++) {
					//res <<= 16;
					/*res +=*/ gf_bs_read_int(bs, 16);
				}
				if (remain) {
					//res <<= remain;
					/* res += */ gf_bs_read_int(bs, remain);
				}
				// entry_point_offset = val + 1; // +1; // +1 to get the size
			}
		}
	}

	if (pps->slice_segment_header_extension_present_flag) {
		u32 size_ext = gf_bs_read_ue_log(bs, "size_ext");
		while (size_ext) {
			gf_bs_read_int(bs, 8);
			size_ext--;
		}
	}

	si->header_size_bits = (gf_bs_get_position(bs) - 1) * 8 + gf_bs_get_bit_position(bs); // av_parser.c modified on 16 jan. 2019 

	if (gf_bs_read_int_log(bs, 1, "byte_align") == 0) {
		GF_LOG(GF_LOG_WARNING, GF_LOG_CODING, ("Error parsing slice header: byte_align not found at end of header !\n"));
	}

	gf_bs_align(bs);
	si->payload_start_offset = (s32)gf_bs_get_position(bs);
	return 0;
}