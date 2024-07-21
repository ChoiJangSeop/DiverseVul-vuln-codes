static s32 gf_hevc_read_vps_bs_internal(GF_BitStream *bs, HEVCState *hevc, Bool stop_at_vps_ext)
{
	u8 vps_sub_layer_ordering_info_present_flag, vps_extension_flag;
	u32 i, j;
	s32 vps_id;
	HEVC_VPS *vps;
	u8 layer_id_included_flag[MAX_LHVC_LAYERS][64];

	//nalu header already parsed
	vps_id = gf_bs_read_int_log(bs, 4, "vps_id");

	if (vps_id >= 16) return -1;

	vps = &hevc->vps[vps_id];
	vps->bit_pos_vps_extensions = -1;
	if (!vps->state) {
		vps->id = vps_id;
		vps->state = 1;
	}

	vps->base_layer_internal_flag = gf_bs_read_int_log(bs, 1, "base_layer_internal_flag");
	vps->base_layer_available_flag = gf_bs_read_int_log(bs, 1, "base_layer_available_flag");
	vps->max_layers = 1 + gf_bs_read_int_log(bs, 6, "max_layers_minus1");
	if (vps->max_layers > MAX_LHVC_LAYERS) {
		GF_LOG(GF_LOG_ERROR, GF_LOG_CODING, ("[HEVC] sorry, %d layers in VPS but only %d supported\n", vps->max_layers, MAX_LHVC_LAYERS));
		return -1;
	}
	vps->max_sub_layers = gf_bs_read_int_log(bs, 3, "max_sub_layers_minus1") + 1;
	vps->temporal_id_nesting = gf_bs_read_int_log(bs, 1, "temporal_id_nesting");
	gf_bs_read_int_log(bs, 16, "vps_reserved_ffff_16bits");
	hevc_profile_tier_level(bs, 1, vps->max_sub_layers - 1, &vps->ptl, 0);

	vps_sub_layer_ordering_info_present_flag = gf_bs_read_int_log(bs, 1, "vps_sub_layer_ordering_info_present_flag");
	for (i = (vps_sub_layer_ordering_info_present_flag ? 0 : vps->max_sub_layers - 1); i < vps->max_sub_layers; i++) {
		gf_bs_read_ue_log_idx(bs, "vps_max_dec_pic_buffering_minus1", i);
		gf_bs_read_ue_log_idx(bs, "vps_max_num_reorder_pics", i);
		gf_bs_read_ue_log_idx(bs, "vps_max_latency_increase_plus1", i);
	}
	vps->max_layer_id = gf_bs_read_int_log(bs, 6, "max_layer_id");
	if (vps->max_layer_id > MAX_LHVC_LAYERS) {
		GF_LOG(GF_LOG_ERROR, GF_LOG_CODING, ("[HEVC] VPS max layer ID %u but GPAC only supports %u\n", vps->max_layer_id, MAX_LHVC_LAYERS));
		return -1;
	}
	vps->num_layer_sets = gf_bs_read_ue_log(bs, "num_layer_sets_minus1") + 1;
	if (vps->num_layer_sets > MAX_LHVC_LAYERS) {
		GF_LOG(GF_LOG_ERROR, GF_LOG_CODING, ("[HEVC] Wrong number of layer sets in VPS %d\n", vps->num_layer_sets));
		return -1;
	}
	for (i = 1; i < vps->num_layer_sets; i++) {
		for (j = 0; j <= vps->max_layer_id; j++) {
			layer_id_included_flag[i][j] = gf_bs_read_int_log_idx2(bs, 1, "layer_id_included_flag", i, j);
		}
	}
	vps->num_layers_in_id_list[0] = 1;
	for (i = 1; i < vps->num_layer_sets; i++) {
		u32 n, m;
		n = 0;
		for (m = 0; m <= vps->max_layer_id; m++) {
			if (layer_id_included_flag[i][m]) {
				vps->LayerSetLayerIdList[i][n++] = m;
				if (vps->LayerSetLayerIdListMax[i] < m)
					vps->LayerSetLayerIdListMax[i] = m;
			}
		}
		vps->num_layers_in_id_list[i] = n;
	}
	if (gf_bs_read_int_log(bs, 1, "vps_timing_info_present_flag")) {
		u32 vps_num_hrd_parameters;
		gf_bs_read_int_log(bs, 32, "vps_num_units_in_tick");
		gf_bs_read_int_log(bs, 32, "vps_time_scale");
		if (gf_bs_read_int_log(bs, 1, "vps_poc_proportional_to_timing_flag")) {
			gf_bs_read_ue_log(bs, "vps_num_ticks_poc_diff_one_minus1");
		}
		vps_num_hrd_parameters = gf_bs_read_ue_log(bs, "vps_num_hrd_parameters");
		for (i = 0; i < vps_num_hrd_parameters; i++) {
			Bool cprms_present_flag = GF_TRUE;
			gf_bs_read_ue_log_idx(bs, "hrd_layer_set_idx", i);
			if (i > 0)
				cprms_present_flag = gf_bs_read_int_log(bs, 1, "cprms_present_flag");
			hevc_parse_hrd_parameters(bs, cprms_present_flag, vps->max_sub_layers - 1, i);
		}
	}
	if (stop_at_vps_ext) {
		return vps_id;
	}

	vps_extension_flag = gf_bs_read_int_log(bs, 1, "vps_extension_flag");
	if (vps_extension_flag) {
		Bool res;
		gf_bs_align(bs);
		res = hevc_parse_vps_extension(vps, bs);
		if (res != GF_TRUE) {
			GF_LOG(GF_LOG_ERROR, GF_LOG_CODING, ("[HEVC] Failed to parse VPS extensions\n"));
			return -1;
		}
		if (gf_bs_read_int_log(bs, 1, "vps_extension2_flag")) {
#if 0
			while (gf_bs_available(bs)) {
				/*vps_extension_data_flag */ gf_bs_read_int(bs, 1);
			}
#endif

		}
	}
	return vps_id;
}