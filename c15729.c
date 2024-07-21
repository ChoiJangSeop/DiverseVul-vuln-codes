static u32 mp4box_cleanup(u32 ret_code) {
	if (mpd_base_urls) {
		gf_free(mpd_base_urls);
		mpd_base_urls = NULL;
	}
	if (sdp_lines) {
		gf_free(sdp_lines);
		sdp_lines = NULL;
	}
	if (metas) {
		u32 i;
		for (i=0; i<nb_meta_act; i++) {
			if (metas[i].enc_type) gf_free(metas[i].enc_type);
			if (metas[i].mime_type) gf_free(metas[i].mime_type);
			if (metas[i].szName) gf_free(metas[i].szName);
			if (metas[i].szPath) gf_free(metas[i].szPath);
		}
		gf_free(metas);
		metas = NULL;
	}
	if (tracks) {
		u32 i;
		for (i = 0; i<nb_track_act; i++) {
			if (tracks[i].out_name)
				gf_free(tracks[i].out_name);
			if (tracks[i].src_name)
				gf_free(tracks[i].src_name);
			if (tracks[i].string)
				gf_free(tracks[i].string);
			if (tracks[i].kind_scheme)
				gf_free(tracks[i].kind_scheme);
			if (tracks[i].kind_value)
				gf_free(tracks[i].kind_value);
		}
		gf_free(tracks);
		tracks = NULL;
	}
	if (tsel_acts) {
		gf_free(tsel_acts);
		tsel_acts = NULL;
	}
	if (brand_add) {
		gf_free(brand_add);
		brand_add = NULL;
	}
	if (brand_rem) {
		gf_free(brand_rem);
		brand_rem = NULL;
	}
	if (dash_inputs) {
		u32 i, j;
		for (i = 0; i<nb_dash_inputs; i++) {
			GF_DashSegmenterInput *di = &dash_inputs[i];
			if (di->nb_baseURL) {
				for (j = 0; j<di->nb_baseURL; j++) {
					gf_free(di->baseURL[j]);
				}
				gf_free(di->baseURL);
			}
			if (di->rep_descs) {
				for (j = 0; j<di->nb_rep_descs; j++) {
					gf_free(di->rep_descs[j]);
				}
				gf_free(di->rep_descs);
			}
			if (di->as_descs) {
				for (j = 0; j<di->nb_as_descs; j++) {
					gf_free(di->as_descs[j]);
				}
				gf_free(di->as_descs);
			}
			if (di->as_c_descs) {
				for (j = 0; j<di->nb_as_c_descs; j++) {
					gf_free(di->as_c_descs[j]);
				}
				gf_free(di->as_c_descs);
			}
			if (di->p_descs) {
				for (j = 0; j<di->nb_p_descs; j++) {
					gf_free(di->p_descs[j]);
				}
				gf_free(di->p_descs);
			}
			if (di->representationID) gf_free(di->representationID);
			if (di->periodID) gf_free(di->periodID);
			if (di->xlink) gf_free(di->xlink);
			if (di->seg_template) gf_free(di->seg_template);
			if (di->hls_pl) gf_free(di->hls_pl);
			if (di->source_opts) gf_free(di->source_opts);
			if (di->filter_chain) gf_free(di->filter_chain);

			if (di->roles) {
				for (j = 0; j<di->nb_roles; j++) {
					gf_free(di->roles[j]);
				}
				gf_free(di->roles);
			}
		}
		gf_free(dash_inputs);
		dash_inputs = NULL;
	}
	if (logfile) gf_fclose(logfile);
	gf_sys_close();
	return ret_code;
}