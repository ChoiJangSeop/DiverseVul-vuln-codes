s32 gf_media_avc_read_sps(const char *sps_data, u32 sps_size, AVCState *avc, u32 subseq_sps, u32 *vui_flag_pos)
{
	AVC_SPS *sps;
	u32 ChromaArrayType = 0;
	s32 mb_width, mb_height, sps_id = -1;
	u32 profile_idc, level_idc, pcomp, i, chroma_format_idc, cl=0, cr=0, ct=0, cb=0, luma_bd, chroma_bd;
	u8 separate_colour_plane_flag = 0;
	GF_BitStream *bs;
	char *sps_data_without_emulation_bytes = NULL;
	u32 sps_data_without_emulation_bytes_size = 0;

	/*SPS still contains emulation bytes*/
	sps_data_without_emulation_bytes = gf_malloc(sps_size*sizeof(char));
	sps_data_without_emulation_bytes_size = avc_remove_emulation_bytes(sps_data, sps_data_without_emulation_bytes, sps_size);
	bs = gf_bs_new(sps_data_without_emulation_bytes, sps_data_without_emulation_bytes_size, GF_BITSTREAM_READ);
	if (!bs) {
		sps_id = -1;
		goto exit;
	}
	if (vui_flag_pos) *vui_flag_pos = 0;

	/*nal hdr*/ gf_bs_read_int(bs, 8);

	profile_idc = gf_bs_read_int(bs, 8);

	pcomp = gf_bs_read_int(bs, 8);
	/*sanity checks*/
	if (pcomp & 0x3)
		goto exit;

	level_idc = gf_bs_read_int(bs, 8);

	/*SubsetSps is used to be sure that AVC SPS are not going to be scratched
	by subset SPS. According to the SVC standard, subset SPS can have the same sps_id
	than its base layer, but it does not refer to the same SPS. */
	sps_id = bs_get_ue(bs) + GF_SVC_SSPS_ID_SHIFT * subseq_sps;
	if (sps_id >=32) {
		sps_id = -1;
		goto exit;
	}
	if (sps_id < 0) {
		sps_id = -1;
		goto exit;
	}

	luma_bd = chroma_bd = 0;
	chroma_format_idc = ChromaArrayType = 1;
	sps = &avc->sps[sps_id];
	sps->state |= subseq_sps ? AVC_SUBSPS_PARSED : AVC_SPS_PARSED;

	/*High Profile and SVC*/
	switch (profile_idc) {
	case 100:
	case 110:
	case 122:
	case 244:
	case 44:
		/*sanity checks: note1 from 7.4.2.1.1 of iso/iec 14496-10-N11084*/
		if (pcomp & 0xE0)
			goto exit;
	case 83:
	case 86:
	case 118:
	case 128:
		chroma_format_idc = bs_get_ue(bs);
		ChromaArrayType = chroma_format_idc;
		if (chroma_format_idc == 3) {
			separate_colour_plane_flag = gf_bs_read_int(bs, 1);
			/*
			Depending on the value of separate_colour_plane_flag, the value of the variable ChromaArrayType is assigned as follows.
			\96	If separate_colour_plane_flag is equal to 0, ChromaArrayType is set equal to chroma_format_idc.
			\96	Otherwise (separate_colour_plane_flag is equal to 1), ChromaArrayType is set equal to 0.
			*/
			if (separate_colour_plane_flag) ChromaArrayType = 0;
		}
		luma_bd = bs_get_ue(bs);
		chroma_bd = bs_get_ue(bs);
		/*qpprime_y_zero_transform_bypass_flag = */ gf_bs_read_int(bs, 1);
		/*seq_scaling_matrix_present_flag*/
		if (gf_bs_read_int(bs, 1)) {
			u32 k;
			for (k=0; k<8; k++) {
				if (gf_bs_read_int(bs, 1)) {
					u32 z, last = 8, next = 8;
					u32 sl = k<6 ? 16 : 64;
					for (z=0; z<sl; z++) {
						if (next) {
							s32 delta = bs_get_se(bs);
							next = (last + delta + 256) % 256;
						}
						last = next ? next : last;
					}
				}
			}
		}
		break;
	}

	sps->profile_idc = profile_idc;
	sps->level_idc = level_idc;
	sps->prof_compat = pcomp;
	sps->log2_max_frame_num = bs_get_ue(bs) + 4;
	sps->poc_type = bs_get_ue(bs);
	sps->chroma_format = chroma_format_idc;
	sps->luma_bit_depth_m8 = luma_bd;
	sps->chroma_bit_depth_m8 = chroma_bd;

	if (sps->poc_type == 0) {
		sps->log2_max_poc_lsb = bs_get_ue(bs) + 4;
	} else if(sps->poc_type == 1) {
		sps->delta_pic_order_always_zero_flag = gf_bs_read_int(bs, 1);
		sps->offset_for_non_ref_pic = bs_get_se(bs);
		sps->offset_for_top_to_bottom_field = bs_get_se(bs);
		sps->poc_cycle_length = bs_get_ue(bs);
		for(i=0; i<sps->poc_cycle_length; i++) sps->offset_for_ref_frame[i] = bs_get_se(bs);
	}
	if (sps->poc_type > 2) {
		sps_id = -1;
		goto exit;
	}
	sps->max_num_ref_frames = bs_get_ue(bs); 
	sps->gaps_in_frame_num_value_allowed_flag = gf_bs_read_int(bs, 1); 
	mb_width = bs_get_ue(bs) + 1;
	mb_height= bs_get_ue(bs) + 1;

	sps->frame_mbs_only_flag = gf_bs_read_int(bs, 1);

	sps->width = mb_width * 16;
	sps->height = (2-sps->frame_mbs_only_flag) * mb_height * 16;
	
	if (!sps->frame_mbs_only_flag) sps->mb_adaptive_frame_field_flag = gf_bs_read_int(bs, 1);
	gf_bs_read_int(bs, 1); /*direct_8x8_inference_flag*/
	
	if (gf_bs_read_int(bs, 1)) { /*crop*/
		int CropUnitX, CropUnitY, SubWidthC = -1, SubHeightC = -1;

		if (chroma_format_idc == 1) {
			SubWidthC = 2, SubHeightC = 2;
		} else if (chroma_format_idc == 2) {
			SubWidthC = 2, SubHeightC = 1;
		} else if ((chroma_format_idc == 3) && (separate_colour_plane_flag == 0)) {
			SubWidthC = 1, SubHeightC = 1;
		}

		if (ChromaArrayType == 0) {
			assert(SubWidthC==-1);
			CropUnitX = 1;
			CropUnitY = 2-sps->frame_mbs_only_flag;
		} else {
			CropUnitX = SubWidthC;
			CropUnitY = SubHeightC * (2-sps->frame_mbs_only_flag);
		}

		cl = bs_get_ue(bs); /*crop_left*/
		cr = bs_get_ue(bs); /*crop_right*/
		ct = bs_get_ue(bs); /*crop_top*/
		cb = bs_get_ue(bs); /*crop_bottom*/

		sps->width -= CropUnitX * (cl + cr);
		sps->height -= CropUnitY * (ct + cb);
		cl *= CropUnitX;
		cr *= CropUnitX;
		ct *= CropUnitY;
		cb *= CropUnitY;
	} 
	sps->crop.left = cl;
	sps->crop.right = cr;
	sps->crop.top = ct;
	sps->crop.bottom = cb;

	if (vui_flag_pos) {
		*vui_flag_pos = (u32) gf_bs_get_bit_offset(bs);
	}
	/*vui_parameters_present_flag*/
	sps->vui_parameters_present_flag = gf_bs_read_int(bs, 1);
	if (sps->vui_parameters_present_flag) {
		sps->vui.aspect_ratio_info_present_flag = gf_bs_read_int(bs, 1);
		if (sps->vui.aspect_ratio_info_present_flag) {
			s32 aspect_ratio_idc = gf_bs_read_int(bs, 8);
			if (aspect_ratio_idc == 255) {
				sps->vui.par_num = gf_bs_read_int(bs, 16); /*AR num*/
				sps->vui.par_den = gf_bs_read_int(bs, 16); /*AR den*/
			} else if (aspect_ratio_idc<14) {
				sps->vui.par_num = avc_sar[aspect_ratio_idc].w;
				sps->vui.par_den = avc_sar[aspect_ratio_idc].h;
			}
		}
		sps->vui.overscan_info_present_flag = gf_bs_read_int(bs, 1);
		if(sps->vui.overscan_info_present_flag)		
			gf_bs_read_int(bs, 1);		/* overscan_appropriate_flag */

		/* default values */
		sps->vui.video_format = 5;
		sps->vui.colour_primaries = 2;
		sps->vui.transfer_characteristics = 2;
		sps->vui.matrix_coefficients = 2;
		/* now read values if possible */
		sps->vui.video_signal_type_present_flag = gf_bs_read_int(bs, 1);
		if (sps->vui.video_signal_type_present_flag) {
			sps->vui.video_format = gf_bs_read_int(bs, 3);
			sps->vui.video_full_range_flag = gf_bs_read_int(bs, 1);
			sps->vui.colour_description_present_flag = gf_bs_read_int(bs, 1);
			if (sps->vui.colour_description_present_flag) { 
				sps->vui.colour_primaries = gf_bs_read_int(bs, 8);  
				sps->vui.transfer_characteristics = gf_bs_read_int(bs, 8);  
				sps->vui.matrix_coefficients = gf_bs_read_int(bs, 8);  
			}
		}

		if (gf_bs_read_int(bs, 1)) {	/* chroma_location_info_present_flag */
			bs_get_ue(bs);				/* chroma_sample_location_type_top_field */
			bs_get_ue(bs);				/* chroma_sample_location_type_bottom_field */
		}

		sps->vui.timing_info_present_flag = gf_bs_read_int(bs, 1);
		if (sps->vui.timing_info_present_flag) {
			sps->vui.num_units_in_tick = gf_bs_read_int(bs, 32);
			sps->vui.time_scale = gf_bs_read_int(bs, 32);
			sps->vui.fixed_frame_rate_flag = gf_bs_read_int(bs, 1);
		}

		sps->vui.nal_hrd_parameters_present_flag = gf_bs_read_int(bs, 1);
		if (sps->vui.nal_hrd_parameters_present_flag)
			avc_parse_hrd_parameters(bs, &sps->vui.hrd);

		sps->vui.vcl_hrd_parameters_present_flag = gf_bs_read_int(bs, 1);
		if (sps->vui.vcl_hrd_parameters_present_flag)
			avc_parse_hrd_parameters(bs, &sps->vui.hrd);

		if (sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag)
			sps->vui.low_delay_hrd_flag = gf_bs_read_int(bs, 1); 

		sps->vui.pic_struct_present_flag = gf_bs_read_int(bs, 1);
	}
	/*end of seq_parameter_set_data*/

	if (subseq_sps) {
		if ((profile_idc==83) || (profile_idc==86)) {
			u8 extended_spatial_scalability_idc;
			/*parsing seq_parameter_set_svc_extension*/

			/*inter_layer_deblocking_filter_control_present_flag=*/	gf_bs_read_int(bs, 1);
			extended_spatial_scalability_idc = gf_bs_read_int(bs, 2);
			if (ChromaArrayType == 1 || ChromaArrayType == 2) {
				/*chroma_phase_x_plus1_flag*/ gf_bs_read_int(bs, 1);
			}
			if( ChromaArrayType  ==  1 ) {
				/*chroma_phase_y_plus1*/ gf_bs_read_int(bs, 2);
			}
			if (extended_spatial_scalability_idc == 1) {
				if( ChromaArrayType > 0 ) {
					/*seq_ref_layer_chroma_phase_x_plus1_flag*/gf_bs_read_int(bs, 1);
					/*seq_ref_layer_chroma_phase_y_plus1*/gf_bs_read_int(bs, 2);
				}
				/*seq_scaled_ref_layer_left_offset*/ bs_get_se(bs);
				/*seq_scaled_ref_layer_top_offset*/bs_get_se(bs);
				/*seq_scaled_ref_layer_right_offset*/bs_get_se(bs);
				/*seq_scaled_ref_layer_bottom_offset*/bs_get_se(bs);
			}
			if (/*seq_tcoeff_level_prediction_flag*/gf_bs_read_int(bs, 1)) {
				/*adaptive_tcoeff_level_prediction_flag*/ gf_bs_read_int(bs, 1);
			}
			/*slice_header_restriction_flag*/gf_bs_read_int(bs, 1);

			/*svc_vui_parameters_present*/
			if (gf_bs_read_int(bs, 1)) {
				u32 i, vui_ext_num_entries_minus1;
				vui_ext_num_entries_minus1 = bs_get_ue(bs);

				for (i=0; i <= vui_ext_num_entries_minus1; i++) {
					u8 vui_ext_nal_hrd_parameters_present_flag, vui_ext_vcl_hrd_parameters_present_flag, vui_ext_timing_info_present_flag;
					/*u8 vui_ext_dependency_id =*/ gf_bs_read_int(bs, 3);
					/*u8 vui_ext_quality_id =*/ gf_bs_read_int(bs, 4);
					/*u8 vui_ext_temporal_id =*/ gf_bs_read_int(bs, 3);
					vui_ext_timing_info_present_flag = gf_bs_read_int(bs, 1);
					if (vui_ext_timing_info_present_flag) {
						/*u32 vui_ext_num_units_in_tick = */gf_bs_read_int(bs, 32);
						/*u32 vui_ext_time_scale = */gf_bs_read_int(bs, 32);
						/*u8 vui_ext_fixed_frame_rate_flag = */gf_bs_read_int(bs, 1);
					}
					vui_ext_nal_hrd_parameters_present_flag = gf_bs_read_int(bs, 1);
					if (vui_ext_nal_hrd_parameters_present_flag) {
						//hrd_parameters( )
					}
					vui_ext_vcl_hrd_parameters_present_flag = gf_bs_read_int(bs, 1);
					if (vui_ext_vcl_hrd_parameters_present_flag) {
						//hrd_parameters( )
					}
					if ( vui_ext_nal_hrd_parameters_present_flag ||  vui_ext_vcl_hrd_parameters_present_flag) {
						/*vui_ext_low_delay_hrd_flag*/gf_bs_read_int(bs, 1);
					}
					/*vui_ext_pic_struct_present_flag*/gf_bs_read_int(bs, 1);
				}
			}
		}
		else if ((profile_idc==118) || (profile_idc==128)) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_CODING, ("[avc-h264] MVC not supported - skipping parsing end of Subset SPS\n"));
			goto exit;
		}

		if (gf_bs_read_int(bs, 1)) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_CODING, ("[avc-h264] skipping parsing end of Subset SPS (additional_extension2)\n"));
			goto exit;
		}
	}

exit:
	gf_bs_del(bs);
	gf_free(sps_data_without_emulation_bytes);
	return sps_id;
}