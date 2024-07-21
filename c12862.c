GF_Err import_file(GF_ISOFile *dest, char *inName, u32 import_flags, GF_Fraction force_fps, u32 frames_per_sample, GF_FilterSession *fsess, char **mux_args_if_first_pass, u32 tk_idx)
{
	u32 track_id, i, j, timescale, track, stype, profile, compat, level, new_timescale, rescale_num, rescale_den, svc_mode, txt_flags, split_tile_mode, temporal_mode, nb_tracks;
	s32 par_d, par_n, prog_id, force_rate, moov_timescale;
	s32 tw, th, tx, ty, tz, txtw, txth, txtx, txty;
	Bool do_audio, do_video, do_auxv,do_pict, do_all, track_layout, text_layout, chap_ref, is_chap, is_chap_file, keep_handler, rap_only, refs_only, force_par, rewrite_bs;
	u32 group, handler, rvc_predefined, check_track_for_svc, check_track_for_lhvc, check_track_for_hevc, do_disable;
	const char *szLan;
	GF_Err e = GF_OK;
	GF_Fraction delay;
	u32 tmcd_track = 0, neg_ctts_mode=0;
	Bool keep_audelim = GF_FALSE;
	u32 print_stats_graph=fs_dump_flags;
	GF_MediaImporter import;
	char *ext, szName[1000], *handler_name, *rvc_config, *chapter_name;
	GF_List *kinds;
	GF_TextFlagsMode txt_mode = GF_ISOM_TEXT_FLAGS_OVERWRITE;
	u8 max_layer_id_plus_one, max_temporal_id_plus_one;
	u32 clap_wn, clap_wd, clap_hn, clap_hd, clap_hon, clap_hod, clap_von, clap_vod;
	Bool has_clap=GF_FALSE;
	Bool use_stz2=GF_FALSE;
	Bool has_mx=GF_FALSE;
	s32 mx[9];
	u32 bitdepth=0;
	u32 dv_profile=0; /*Dolby Vision*/
	u32 clr_type=0;
	u32 clr_prim;
	u32 clr_tranf;
	u32 clr_mx;
	Bool rescale_override=GF_FALSE;
	Bool clr_full_range=GF_FALSE;
	Bool fmt_ok = GF_TRUE;
	u32 icc_size=0, track_flags=0;
	u8 *icc_data = NULL;
	u32 tc_fps_num=0, tc_fps_den=0, tc_h=0, tc_m=0, tc_s=0, tc_f=0, tc_frames_per_tick=0;
	Bool tc_force_counter=GF_FALSE;
	Bool tc_drop_frame = GF_FALSE;
	char *ext_start;
	u32 xps_inband=0;
	u64 source_magic=0;
	char *opt_src = NULL;
	char *opt_dst = NULL;
	char *fchain = NULL;
	char *edits = NULL;
	const char *fail_msg = NULL;
	Bool set_ccst=GF_FALSE;
	Bool has_last_sample_dur=GF_FALSE;
	u32 fake_import = 0;
	GF_Fraction last_sample_dur = {0,0};
	s32 fullrange, videofmt, colorprim, colortfc, colormx;
	clap_wn = clap_wd = clap_hn = clap_hd = clap_hon = clap_hod = clap_von = clap_vod = 0;
	GF_ISOMTrackFlagOp track_flags_mode=0;
	u32 roll_change=0;
	u32 roll = 0;
	Bool src_is_isom = GF_FALSE;

	rvc_predefined = 0;
	chapter_name = NULL;
	new_timescale = 1;
	moov_timescale = 0;
	rescale_num = rescale_den = 0;
	text_layout = 0;
	/*0: merge all
	  1: split base and all SVC in two tracks
	  2: split all base and SVC layers in dedicated tracks
	 */
	svc_mode = 0;

	if (import_flags==0xFFFFFFFF) {
		import_flags = 0;
		fake_import = 1;
	}

	memset(&import, 0, sizeof(GF_MediaImporter));

	strcpy(szName, inName);
#ifdef WIN32
	/*dirty hack for msys&mingw: when we use import options, the ':' separator used prevents msys from translating the path
	we do this for regular cases where the path starts with the drive letter. If the path start with anything else (/home , /opt, ...) we're screwed :( */
	if ( (szName[0]=='/') && (szName[2]=='/')) {
		szName[0] = szName[1];
		szName[1] = ':';
	}
#endif

	is_chap_file = 0;
	handler = 0;
	do_disable = 0;
	chap_ref = 0;
	is_chap = 0;
	kinds = gf_list_new();
	track_layout = 0;
	szLan = NULL;
	delay.num = delay.den = 0;
	group = 0;
	stype = 0;
	profile = compat = level = 0;
	fullrange = videofmt = colorprim = colortfc = colormx = -1;
	split_tile_mode = 0;
	temporal_mode = 0;
	rap_only = 0;
	refs_only = 0;
	txt_flags = 0;
	max_layer_id_plus_one = max_temporal_id_plus_one = 0;
	force_rate = -1;

	tw = th = tx = ty = tz = txtw = txth = txtx = txty = 0;
	par_d = par_n = -1;
	force_par = rewrite_bs = GF_FALSE;


	ext_start = gf_file_ext_start(szName);
	ext = strrchr(ext_start ? ext_start : szName, '#');
	if (!ext) ext = gf_url_colon_suffix(szName);
	char c_sep = ext ? ext[0] : 0;
	if (ext) ext[0] = 0;
 	if (!strlen(szName) || !strcmp(szName, "self")) {
		fake_import = 2;
	}
	if (gf_isom_probe_file(szName))
		src_is_isom = GF_TRUE;

	if (ext) ext[0] = c_sep;

	ext = gf_url_colon_suffix(szName);

#define GOTO_EXIT(_msg) if (e) { fail_msg = _msg; goto exit; }

#define CHECK_FAKEIMPORT(_opt) if (fake_import) { M4_LOG(GF_LOG_ERROR, ("Option %s not available for self-reference import\n", _opt)); e = GF_BAD_PARAM; goto exit; }
#define CHECK_FAKEIMPORT_2(_opt) if (fake_import==1) { M4_LOG(GF_LOG_ERROR, ("Option %s not available for self-reference import\n", _opt)); e = GF_BAD_PARAM; goto exit; }


	handler_name = NULL;
	rvc_config = NULL;
	while (ext) {
		char *ext2 = gf_url_colon_suffix(ext+1);

		if (ext2) ext2[0] = 0;

		/*all extensions for track-based importing*/
		if (!strnicmp(ext+1, "dur=", 4)) {
			CHECK_FAKEIMPORT("dur")

			if (strchr(ext, '-')) {
				import.duration.num = atoi(ext+5);
				import.duration.den = 1;
			} else {
				gf_parse_frac(ext+5, &import.duration);
			}
		}
		else if (!strnicmp(ext+1, "start=", 6)) {
			CHECK_FAKEIMPORT("start")
			import.start_time = atof(ext+7);
		}
		else if (!strnicmp(ext+1, "lang=", 5)) {
			/* prevent leak if param is set twice */
			if (szLan)
				gf_free((char*) szLan);

			szLan = gf_strdup(ext+6);
		}
		else if (!strnicmp(ext+1, "delay=", 6)) {
			if (sscanf(ext+7, "%d/%u", &delay.num, &delay.den)!=2) {
				delay.num = atoi(ext+7);
				delay.den = 0;
			}
		}
		else if (!strnicmp(ext+1, "par=", 4)) {
			if (!stricmp(ext + 5, "none")) {
				par_n = par_d = 0;
			} else if (!stricmp(ext + 5, "auto")) {
				force_par = GF_TRUE;
			} else if (!stricmp(ext + 5, "force")) {
				par_n = par_d = 1;
				force_par = GF_TRUE;
			} else {
				if (ext2) {
					ext2[0] = ':';
					ext2 = strchr(ext2+1, ':');
					if (ext2) ext2[0] = 0;
				}
				if (ext[5]=='w') {
					rewrite_bs = GF_TRUE;
					sscanf(ext+6, "%d:%d", &par_n, &par_d);
				} else {
					sscanf(ext+5, "%d:%d", &par_n, &par_d);
				}
			}
		}
		else if (!strnicmp(ext+1, "clap=", 5)) {
			if (!stricmp(ext+6, "none")) {
				has_clap=GF_TRUE;
			} else {
				if (sscanf(ext+6, "%d,%d,%d,%d,%d,%d,%d,%d", &clap_wn, &clap_wd, &clap_hn, &clap_hd, &clap_hon, &clap_hod, &clap_von, &clap_vod)==8) {
					has_clap=GF_TRUE;
				}
			}
		}
		else if (!strnicmp(ext+1, "mx=", 3)) {
			if (strstr(ext+4, "0x")) {
				if (sscanf(ext+4, "0x%d:0x%d:0x%d:0x%d:0x%d:0x%d:0x%d:0x%d:0x%d", &mx[0], &mx[1], &mx[2], &mx[3], &mx[4], &mx[5], &mx[6], &mx[7], &mx[8])==9) {
					has_mx=GF_TRUE;
				}
			} else if (sscanf(ext+4, "%d:%d:%d:%d:%d:%d:%d:%d:%d", &mx[0], &mx[1], &mx[2], &mx[3], &mx[4], &mx[5], &mx[6], &mx[7], &mx[8])==9) {
				has_mx=GF_TRUE;
			}
		}
		else if (!strnicmp(ext+1, "name=", 5)) {
			handler_name = gf_strdup(ext+6);
		}
		else if (!strnicmp(ext+1, "ext=", 4)) {
			CHECK_FAKEIMPORT("ext")
			/*extensions begin with '.'*/
			if (*(ext+5) == '.')
				import.force_ext = gf_strdup(ext+5);
			else {
				import.force_ext = gf_calloc(1+strlen(ext+5)+1, 1);
				import.force_ext[0] = '.';
				strcat(import.force_ext+1, ext+5);
			}
		}
		else if (!strnicmp(ext+1, "hdlr=", 5)) handler = GF_4CC(ext[6], ext[7], ext[8], ext[9]);
		else if (!strnicmp(ext+1, "tkhd", 4)) {
			char *flags = ext+6;
			if (flags[0]=='+') { track_flags_mode = GF_ISOM_TKFLAGS_ADD; flags += 1; }
			else if (flags[0]=='-') { track_flags_mode = GF_ISOM_TKFLAGS_REM; flags += 1; }
			else track_flags_mode = GF_ISOM_TKFLAGS_SET;

			if (!strnicmp(flags, "0x", 2)) flags += 2;
			sscanf(flags, "%X", &track_flags);
		} else if (!strnicmp(ext+1, "disable", 7)) {
			do_disable = !stricmp(ext+1, "disable=no") ? 2 : 1;
		}
		else if (!strnicmp(ext+1, "group=", 6)) {
			group = atoi(ext+7);
			if (!group) group = gf_isom_get_next_alternate_group_id(dest);
		}
		else if (!strnicmp(ext+1, "fps=", 4)) {
			u32 ticks, dts_inc;
			CHECK_FAKEIMPORT("fps")
			if (!strcmp(ext+5, "auto")) {
				M4_LOG(GF_LOG_ERROR, ("Warning, fps=auto option is deprecated\n"));
			} else if ((sscanf(ext+5, "%u-%u", &ticks, &dts_inc) == 2) || (sscanf(ext+5, "%u/%u", &ticks, &dts_inc) == 2)) {
				if (!dts_inc) dts_inc=1;
				force_fps.num = ticks;
				force_fps.den = dts_inc;
			} else {
				if (gf_sys_old_arch_compat()) {
					force_fps.den = 1000;
					force_fps.num = (u32) (atof(ext+5) * force_fps.den);
				} else {
					gf_parse_frac(ext+5, &force_fps);
				}
			}
		}
		else if (!stricmp(ext+1, "rap")) rap_only = 1;
		else if (!stricmp(ext+1, "refs")) refs_only = 1;
		else if (!stricmp(ext+1, "trailing")) { CHECK_FAKEIMPORT("trailing") import_flags |= GF_IMPORT_KEEP_TRAILING; }
		else if (!strnicmp(ext+1, "agg=", 4)) { CHECK_FAKEIMPORT("agg") frames_per_sample = atoi(ext+5); }
		else if (!stricmp(ext+1, "dref")) { CHECK_FAKEIMPORT("dref")  import_flags |= GF_IMPORT_USE_DATAREF; }
		else if (!stricmp(ext+1, "keep_refs")) { CHECK_FAKEIMPORT("keep_refs") import_flags |= GF_IMPORT_KEEP_REFS; }
		else if (!stricmp(ext+1, "nodrop")) { CHECK_FAKEIMPORT("nodrop") import_flags |= GF_IMPORT_NO_FRAME_DROP; }
		else if (!stricmp(ext+1, "packed")) { CHECK_FAKEIMPORT("packed") import_flags |= GF_IMPORT_FORCE_PACKED; }
		else if (!stricmp(ext+1, "sbr")) { CHECK_FAKEIMPORT("sbr") import_flags |= GF_IMPORT_SBR_IMPLICIT; }
		else if (!stricmp(ext+1, "sbrx")) { CHECK_FAKEIMPORT("sbrx") import_flags |= GF_IMPORT_SBR_EXPLICIT; }
		else if (!stricmp(ext+1, "ovsbr")) { CHECK_FAKEIMPORT("ovsbr") import_flags |= GF_IMPORT_OVSBR; }
		else if (!stricmp(ext+1, "ps")) { CHECK_FAKEIMPORT("ps") import_flags |= GF_IMPORT_PS_IMPLICIT; }
		else if (!stricmp(ext+1, "psx")) { CHECK_FAKEIMPORT("psx") import_flags |= GF_IMPORT_PS_EXPLICIT; }
		else if (!stricmp(ext+1, "mpeg4")) { CHECK_FAKEIMPORT("mpeg4") import_flags |= GF_IMPORT_FORCE_MPEG4; }
		else if (!stricmp(ext+1, "nosei")) { CHECK_FAKEIMPORT("nosei") import_flags |= GF_IMPORT_NO_SEI; }
		else if (!stricmp(ext+1, "svc") || !stricmp(ext+1, "lhvc") ) { CHECK_FAKEIMPORT("svc/lhvc") import_flags |= GF_IMPORT_SVC_EXPLICIT; }
		else if (!stricmp(ext+1, "nosvc") || !stricmp(ext+1, "nolhvc")) { CHECK_FAKEIMPORT("nosvc/nolhvc") import_flags |= GF_IMPORT_SVC_NONE; }

		/*split SVC layers*/
		else if (!strnicmp(ext+1, "svcmode=", 8) || !strnicmp(ext+1, "lhvcmode=", 9)) {
			char *mode = ext+9;
			CHECK_FAKEIMPORT_2("svcmode/lhvcmode")
			if (mode[0]=='=') mode = ext+10;

			if (!stricmp(mode, "splitnox"))
				svc_mode = 3;
			else if (!stricmp(mode, "splitnoxib"))
				svc_mode = 4;
			else if (!stricmp(mode, "splitall") || !stricmp(mode, "split"))
				svc_mode = 2;
			else if (!stricmp(mode, "splitbase"))
				svc_mode = 1;
			else if (!stricmp(mode, "merged") || !stricmp(mode, "merge"))
				svc_mode = 0;
		}
		/*split SHVC temporal sublayers*/
		else if (!strnicmp(ext+1, "temporal=", 9)) {
			char *mode = ext+10;
			CHECK_FAKEIMPORT_2("svcmode/lhvcmode")
			if (!stricmp(mode, "split"))
				temporal_mode = 2;
			else if (!stricmp(mode, "splitnox"))
				temporal_mode = 3;
			else if (!stricmp(mode, "splitbase"))
				temporal_mode = 1;
			else {
				M4_LOG(GF_LOG_ERROR, ("Unrecognized temporal mode %s, ignoring\n", mode));
				temporal_mode = 0;
			}
		}
		else if (!stricmp(ext+1, "subsamples")) { CHECK_FAKEIMPORT("subsamples") import_flags |= GF_IMPORT_SET_SUBSAMPLES; }
		else if (!stricmp(ext+1, "deps")) { CHECK_FAKEIMPORT("deps") import_flags |= GF_IMPORT_SAMPLE_DEPS; }
		else if (!stricmp(ext+1, "ccst")) { CHECK_FAKEIMPORT("ccst") set_ccst = GF_TRUE; }
		else if (!stricmp(ext+1, "alpha")) { CHECK_FAKEIMPORT("alpha") import.is_alpha = GF_TRUE; }
		else if (!stricmp(ext+1, "forcesync")) { CHECK_FAKEIMPORT("forcesync") import_flags |= GF_IMPORT_FORCE_SYNC; }
		else if (!stricmp(ext+1, "xps_inband")) { CHECK_FAKEIMPORT("xps_inband") xps_inband = 1; }
		else if (!stricmp(ext+1, "xps_inbandx")) { CHECK_FAKEIMPORT("xps_inbandx") xps_inband = 2; }
		else if (!stricmp(ext+1, "au_delim")) { CHECK_FAKEIMPORT("au_delim") keep_audelim = GF_TRUE; }
		else if (!strnicmp(ext+1, "max_lid=", 8) || !strnicmp(ext+1, "max_tid=", 8)) {
			s32 val = atoi(ext+9);
			CHECK_FAKEIMPORT_2("max_lid/lhvcmode")
			if (val < 0) {
				M4_LOG(GF_LOG_ERROR, ("Warning: request max layer/temporal id is negative - ignoring\n"));
			} else {
				if (!strnicmp(ext+1, "max_lid=", 8))
					max_layer_id_plus_one = 1 + (u8) val;
				else
					max_temporal_id_plus_one = 1 + (u8) val;
			}
		}
		else if (!stricmp(ext+1, "tiles")) { CHECK_FAKEIMPORT_2("tiles") split_tile_mode = 2; }
		else if (!stricmp(ext+1, "tiles_rle")) { CHECK_FAKEIMPORT_2("tiles_rle") split_tile_mode = 3; }
		else if (!stricmp(ext+1, "split_tiles")) { CHECK_FAKEIMPORT_2("split_tiles") split_tile_mode = 1; }

		/*force all composition offsets to be positive*/
		else if (!strnicmp(ext+1, "negctts", 7)) {
			neg_ctts_mode = !strnicmp(ext+1, "negctts=no", 10) ? 2 : 1;
		}
		else if (!strnicmp(ext+1, "stype=", 6)) {
			stype = GF_4CC(ext[7], ext[8], ext[9], ext[10]);
		}
		else if (!stricmp(ext+1, "chap")) is_chap = 1;
		else if (!strnicmp(ext+1, "chapter=", 8)) {
			chapter_name = gf_strdup(ext+9);
		}
		else if (!strnicmp(ext+1, "chapfile=", 9)) {
			chapter_name = gf_strdup(ext+10);
			is_chap_file=1;
		}
		else if (!strnicmp(ext+1, "layout=", 7)) {
			track_layout = 1;
			if ( sscanf(ext+13, "%dx%dx%dx%dx%d", &tw, &th, &tx, &ty, &tz)==5) {
			} else if ( sscanf(ext+13, "%dx%dx%dx%d", &tw, &th, &tx, &ty)==4) {
				tz = 0;
			} else if ( sscanf(ext+13, "%dx%dx%d", &tw, &th, &tz)==3) {
				tx = ty = 0;
			} else if ( sscanf(ext+8, "%dx%d", &tw, &th)==2) {
				tx = ty = tz = 0;
			}
		}

		else if (!strnicmp(ext+1, "rescale=", 8)) {
			if (sscanf(ext+9, "%u/%u", &rescale_num, &rescale_den) != 2) {
				rescale_num = atoi(ext+9);
				rescale_den = 0;
			}
		}
		else if (!strnicmp(ext+1, "sampdur=", 8)) {
			if (sscanf(ext+9, "%u/%u", &rescale_den, &rescale_num) != 2) {
				rescale_den = atoi(ext+9);
				rescale_num = 0;
			}
			rescale_override = GF_TRUE;
		}
		else if (!strnicmp(ext+1, "timescale=", 10)) {
			new_timescale = atoi(ext+11);
		}
		else if (!strnicmp(ext+1, "moovts=", 7)) {
			moov_timescale = atoi(ext+8);
		}

		else if (!stricmp(ext+1, "noedit")) { import_flags |= GF_IMPORT_NO_EDIT_LIST; }


		else if (!strnicmp(ext+1, "rvc=", 4)) {
			if (sscanf(ext+5, "%d", &rvc_predefined) != 1) {
				rvc_config = gf_strdup(ext+5);
			}
		}
		else if (!strnicmp(ext+1, "fmt=", 4)) import.streamFormat = gf_strdup(ext+5);

		else if (!strnicmp(ext+1, "profile=", 8)) {
			if (!stricmp(ext+9, "high444")) profile = 244;
			else if (!stricmp(ext+9, "high")) profile = 100;
			else if (!stricmp(ext+9, "extended")) profile = 88;
			else if (!stricmp(ext+9, "main")) profile = 77;
			else if (!stricmp(ext+9, "baseline")) profile = 66;
			else profile = atoi(ext+9);
		}
		else if (!strnicmp(ext+1, "level=", 6)) {
			if( atof(ext+7) < 6 )
				level = (int)(10*atof(ext+7)+.5);
			else
				level = atoi(ext+7);
		}
		else if (!strnicmp(ext+1, "compat=", 7)) {
			compat = atoi(ext+8);
		}

		else if (!strnicmp(ext+1, "novpsext", 8)) { CHECK_FAKEIMPORT("novpsext") import_flags |= GF_IMPORT_NO_VPS_EXTENSIONS; }
		else if (!strnicmp(ext+1, "keepav1t", 8)) { CHECK_FAKEIMPORT("keepav1t") import_flags |= GF_IMPORT_KEEP_AV1_TEMPORAL_OBU; }

		else if (!strnicmp(ext+1, "font=", 5)) { CHECK_FAKEIMPORT("font") import.fontName = gf_strdup(ext+6); }
		else if (!strnicmp(ext+1, "size=", 5)) { CHECK_FAKEIMPORT("size") import.fontSize = atoi(ext+6); }
		else if (!strnicmp(ext+1, "text_layout=", 12)) {
			if ( sscanf(ext+13, "%dx%dx%dx%d", &txtw, &txth, &txtx, &txty)==4) {
				text_layout = 1;
			} else if ( sscanf(ext+8, "%dx%d", &txtw, &txth)==2) {
				track_layout = 1;
				txtx = txty = 0;
			}
		}

#ifndef GPAC_DISABLE_SWF_IMPORT
		else if (!stricmp(ext+1, "swf-global")) { CHECK_FAKEIMPORT("swf-global") import.swf_flags |= GF_SM_SWF_STATIC_DICT; }
		else if (!stricmp(ext+1, "swf-no-ctrl")) { CHECK_FAKEIMPORT("swf-no-ctrl") import.swf_flags &= ~GF_SM_SWF_SPLIT_TIMELINE; }
		else if (!stricmp(ext+1, "swf-no-text")) { CHECK_FAKEIMPORT("swf-no-text") import.swf_flags |= GF_SM_SWF_NO_TEXT; }
		else if (!stricmp(ext+1, "swf-no-font")) { CHECK_FAKEIMPORT("swf-no-font") import.swf_flags |= GF_SM_SWF_NO_FONT; }
		else if (!stricmp(ext+1, "swf-no-line")) { CHECK_FAKEIMPORT("swf-no-line") import.swf_flags |= GF_SM_SWF_NO_LINE; }
		else if (!stricmp(ext+1, "swf-no-grad")) { CHECK_FAKEIMPORT("swf-no-grad") import.swf_flags |= GF_SM_SWF_NO_GRADIENT; }
		else if (!stricmp(ext+1, "swf-quad")) { CHECK_FAKEIMPORT("swf-quad") import.swf_flags |= GF_SM_SWF_QUAD_CURVE; }
		else if (!stricmp(ext+1, "swf-xlp")) { CHECK_FAKEIMPORT("swf-xlp") import.swf_flags |= GF_SM_SWF_SCALABLE_LINE; }
		else if (!stricmp(ext+1, "swf-ic2d")) { CHECK_FAKEIMPORT("swf-ic2d") import.swf_flags |= GF_SM_SWF_USE_IC2D; }
		else if (!stricmp(ext+1, "swf-same-app")) { CHECK_FAKEIMPORT("swf-same-app") import.swf_flags |= GF_SM_SWF_REUSE_APPEARANCE; }
		else if (!strnicmp(ext+1, "swf-flatten=", 12)) { CHECK_FAKEIMPORT("swf-flatten") import.swf_flatten_angle = (Float) atof(ext+13); }
#endif

		else if (!strnicmp(ext+1, "kind=", 5)) {
			char *kind_scheme, *kind_value;
			char *kind_data = ext+6;
			char *sep = strchr(kind_data, '=');
			if (sep) {
				*sep = 0;
			}
			kind_scheme = gf_strdup(kind_data);
			if (sep) {
				*sep = '=';
				kind_value = gf_strdup(sep+1);
			} else {
				kind_value = NULL;
			}
			gf_list_add(kinds, kind_scheme);
			gf_list_add(kinds, kind_value);
		}
		else if (!strnicmp(ext+1, "txtflags", 8)) {
			if (!strnicmp(ext+1, "txtflags=", 9)) {
				sscanf(ext+10, "%x", &txt_flags);
			}
			else if (!strnicmp(ext+1, "txtflags+=", 10)) {
				sscanf(ext+11, "%x", &txt_flags);
				txt_mode = GF_ISOM_TEXT_FLAGS_TOGGLE;
			}
			else if (!strnicmp(ext+1, "txtflags-=", 10)) {
				sscanf(ext+11, "%x", &txt_flags);
				txt_mode = GF_ISOM_TEXT_FLAGS_UNTOGGLE;
			}
		}
		else if (!strnicmp(ext+1, "rate=", 5)) {
			force_rate = atoi(ext+6);
		}
		else if (!stricmp(ext+1, "stats") || !stricmp(ext+1, "fstat"))
			print_stats_graph |= 1;
		else if (!stricmp(ext+1, "graph") || !stricmp(ext+1, "graph"))
			print_stats_graph |= 2;
		else if (!strncmp(ext+1, "sopt", 4) || !strncmp(ext+1, "dopt", 4) || !strncmp(ext+1, "@", 1)) {
			if (ext2) ext2[0] = ':';
			opt_src = strstr(ext, ":sopt:");
			opt_dst = strstr(ext, ":dopt:");
			fchain = strstr(ext, ":@");
			if (opt_src) opt_src[0] = 0;
			if (opt_dst) opt_dst[0] = 0;
			if (fchain) fchain[0] = 0;

			if (opt_src) import.filter_src_opts = opt_src+6;
			if (opt_dst) import.filter_dst_opts = opt_dst+6;
			if (fchain) {
				//check for old syntax (0.9->1.0) :@@
				import.filter_chain = fchain + ((fchain[2]=='@') ? 3 : 2);
			}

			ext = NULL;
			break;
		}

		else if (!strnicmp(ext+1, "asemode=", 8)){
			char *mode = ext+9;
			if (!stricmp(mode, "v0-bs"))
				import.asemode = GF_IMPORT_AUDIO_SAMPLE_ENTRY_v0_BS;
			else if (!stricmp(mode, "v0-2"))
				import.asemode = GF_IMPORT_AUDIO_SAMPLE_ENTRY_v0_2;
			else if (!stricmp(mode, "v1"))
				import.asemode = GF_IMPORT_AUDIO_SAMPLE_ENTRY_v1_MPEG;
			else if (!stricmp(mode, "v1-qt"))
				import.asemode = GF_IMPORT_AUDIO_SAMPLE_ENTRY_v1_QTFF;
			else
				M4_LOG(GF_LOG_ERROR, ("Unrecognized audio sample entry mode %s, ignoring\n", mode));
		}
		else if (!strnicmp(ext+1, "audio_roll=", 11)) { roll_change = 3; roll = atoi(ext+12); }
		else if (!strnicmp(ext+1, "roll=", 5)) { roll_change = 1; roll = atoi(ext+6); }
		else if (!strnicmp(ext+1, "proll=", 6)) { roll_change = 2; roll = atoi(ext+7); }
		else if (!strcmp(ext+1, "stz2")) {
			use_stz2 = GF_TRUE;
		} else if (!strnicmp(ext+1, "bitdepth=", 9)) {
			bitdepth=atoi(ext+10);
		}
		else if (!strnicmp(ext+1, "colr=", 5)) {
			char *cval = ext+6;
			if (strlen(cval)<6) {
				fmt_ok = GF_FALSE;
			} else {
				clr_type = GF_4CC(cval[0],cval[1],cval[2],cval[3]);
				cval+=4;
				if (cval[0] != ',') {
					fmt_ok = GF_FALSE;
				}
				else if ((clr_type==GF_ISOM_SUBTYPE_NCLX) || (clr_type==GF_ISOM_SUBTYPE_NCLC)) {
					fmt_ok = scan_color(cval+1, &clr_prim, &clr_tranf, &clr_mx, &clr_full_range);
				}
				else if ((clr_type==GF_ISOM_SUBTYPE_RICC) || (clr_type==GF_ISOM_SUBTYPE_PROF)) {
					FILE *f = gf_fopen(cval+1, "rb");
					if (!f) {
						M4_LOG(GF_LOG_ERROR, ("Failed to open file %s\n", cval+1));
						e = GF_BAD_PARAM;
						goto exit;
					} else {
						gf_fseek(f, 0, SEEK_END);
						icc_size = (u32) gf_ftell(f);
						icc_data = gf_malloc(sizeof(char)*icc_size);
						gf_fseek(f, 0, SEEK_SET);
						icc_size = (u32) gf_fread(icc_data, icc_size, f);
						gf_fclose(f);
					}
				} else {
					M4_LOG(GF_LOG_ERROR, ("Unrecognized colr profile %s\n", gf_4cc_to_str(clr_type) ));
					e = GF_BAD_PARAM;
					goto exit;
				}
			}
			if (!fmt_ok) {
				e = GF_BAD_PARAM;
				GOTO_EXIT("parsing colr option");
			}
		}
		else if (!strnicmp(ext + 1, "dv-profile=", 11)) {
			dv_profile = atoi(ext + 12);
		}
		else if (!strnicmp(ext+1, "fullrange=", 10)) {
			if (!stricmp(ext+11, "off") || !stricmp(ext+11, "no")) fullrange = 0;
			else if (!stricmp(ext+11, "on") || !stricmp(ext+11, "yes")) fullrange = 1;
			else {
				e = GF_BAD_PARAM;
				GOTO_EXIT("invalid format for fullrange")
			}
		}
		else if (!strnicmp(ext+1, "videofmt=", 10)) {
			u32 idx, count = GF_ARRAY_LENGTH(videofmt_names);
			for (idx=0; idx<count; idx++) {
				if (!strcmp(ext+11, videofmt_names[idx])) {
					videofmt = idx;
					break;
				}
			}
			if (videofmt==-1) {
				e = GF_BAD_PARAM;
				GOTO_EXIT("invalid format for videofmt")
			}
		}
		else if (!strnicmp(ext+1, "colorprim=", 10)) {
			colorprim = gf_cicp_parse_color_primaries(ext+11);
			if (colorprim==-1) {
				e = GF_BAD_PARAM;
				GOTO_EXIT("invalid format for colorprim")
			}
		}
		else if (!strnicmp(ext+1, "colortfc=", 9)) {
			colortfc = gf_cicp_parse_color_transfer(ext+10);
			if (colortfc==-1) {
				e = GF_BAD_PARAM;
				GOTO_EXIT("invalid format for colortfc")
			}
		}
		else if (!strnicmp(ext+1, "colormx=", 10)) {
			colormx = gf_cicp_parse_color_matrix(ext+11);
			if (colormx==-1) {
				e = GF_BAD_PARAM;
				GOTO_EXIT("invalid format for colormx")
			}
		}
		else if (!strnicmp(ext+1, "tc=", 3)) {
			char *tc_str = ext+4;
			
			if (tc_str[0] == 'd') {
				tc_drop_frame=GF_TRUE;
				tc_str+=1;
			}
			if (sscanf(tc_str, "%d/%d,%d,%d,%d,%d,%d", &tc_fps_num, &tc_fps_den, &tc_h, &tc_m, &tc_s, &tc_f, &tc_frames_per_tick) == 7) {
			} else if (sscanf(tc_str, "%d/%d,%d,%d,%d,%d", &tc_fps_num, &tc_fps_den, &tc_h, &tc_m, &tc_s, &tc_f) == 6) {
			} else if (sscanf(tc_str, "%d,%d,%d,%d,%d,%d", &tc_fps_num, &tc_h, &tc_m, &tc_s, &tc_f, &tc_frames_per_tick) == 6) {
				tc_fps_den = 1;
			} else if (sscanf(tc_str, "%d,%d,%d,%d,%d", &tc_fps_num, &tc_h, &tc_m, &tc_s, &tc_f) == 5) {
				tc_fps_den = 1;
			} else if (sscanf(tc_str, "%d/%d,%d,%d", &tc_fps_num, &tc_fps_den, &tc_f, &tc_frames_per_tick) == 4) {
				tc_force_counter = GF_TRUE;
				tc_h = tc_m = tc_s = 0;
			} else if (sscanf(tc_str, "%d/%d,%d", &tc_fps_num, &tc_fps_den, &tc_f) == 3) {
				tc_force_counter = GF_TRUE;
				tc_h = tc_m = tc_s = 0;
			} else if (sscanf(tc_str, "%d,%d,%d", &tc_fps_num, &tc_f, &tc_frames_per_tick) == 3) {
				tc_force_counter = GF_TRUE;
				tc_h = tc_m = tc_s = 0;
				tc_fps_den = 1;
			} else if (sscanf(tc_str, "%d,%d", &tc_fps_num, &tc_f) == 2) {
				tc_force_counter = GF_TRUE;
				tc_h = tc_m = tc_s = 0;
				tc_fps_den = 1;
			} else {
				M4_LOG(GF_LOG_ERROR, ("Bad format %s for timecode, ignoring\n", ext+1));
			}
		}
		else if (!strnicmp(ext+1, "edits=", 6)) {
			edits = gf_strdup(ext+7);
		}
		else if (!strnicmp(ext+1, "lastsampdur", 11)) {
			has_last_sample_dur = GF_TRUE;
			if (!strnicmp(ext+1, "lastsampdur=", 12)) {
				if (sscanf(ext+13, "%d/%u", &last_sample_dur.num, &last_sample_dur.den)==2) {
				} else {
					last_sample_dur.num = atoi(ext+13);
					last_sample_dur.den = 1000;
				}
			}
		}
		/*unrecognized, assume name has colon in it*/
		else {
			M4_LOG(GF_LOG_ERROR, ("Unrecognized import option %s, ignoring\n", ext+1));
			if (ext2) ext2[0] = ':';
			ext = ext2;
			continue;
		}
		if (src_is_isom) {
			char *opt = ext+1;
			char *sep_eq = strchr(opt, '=');
			if (sep_eq) sep_eq[0] = 0;
			if (!mp4box_check_isom_fileopt(opt)) {
				M4_LOG(GF_LOG_ERROR, ("\t! Import option `%s` not available for ISOBMFF/QT sources, ignoring !\n", ext+1));
			}
			if (sep_eq) sep_eq[0] = '=';
		}

		if (ext2) ext2[0] = ':';

		ext[0] = 0;

		/* restart from where we stopped
		 * if we didn't stop (ext2 null) then the end has been reached
		 * so we can stop the whole thing */
		ext = ext2;
	}

	/*check duration import (old syntax)*/
	ext = strrchr(szName, '%');
	if (ext) {
		gf_parse_frac(ext+1, &import.duration);
		ext[0] = 0;
	}

	/*select switches for av containers import*/
	do_audio = do_video = do_auxv = do_pict = 0;
	track_id = prog_id = 0;
	do_all = 1;

	ext_start = gf_file_ext_start(szName);
	ext = strrchr(ext_start ? ext_start : szName, '#');
	if (ext) ext[0] = 0;

	if (fake_import && ext) {
		ext++;
		if (!strnicmp(ext, "audio", 5)) do_audio = 1;
		else if (!strnicmp(ext, "video", 5)) do_video = 1;
		else if (!strnicmp(ext, "auxv", 4)) do_auxv = 1;
		else if (!strnicmp(ext, "pict", 4)) do_pict = 1;
		else if (!strnicmp(ext, "trackID=", 8)) track_id = atoi(&ext[8]);
		else track_id = atoi(ext);
	}
	else if (ext) {
		ext++;
		char *sep = gf_url_colon_suffix(ext);
		if (sep) sep[0] = 0;

		//we have a fragment, we need to check if the track or the program is present in source
		import.in_name = szName;
		import.flags = GF_IMPORT_PROBE_ONLY;
		e = gf_media_import(&import);
		GOTO_EXIT("importing import");

		if (!strnicmp(ext, "audio", 5)) do_audio = 1;
		else if (!strnicmp(ext, "video", 5)) do_video = 1;
        else if (!strnicmp(ext, "auxv", 4)) do_auxv = 1;
        else if (!strnicmp(ext, "pict", 4)) do_pict = 1;
		else if (!strnicmp(ext, "trackID=", 8)) track_id = atoi(&ext[8]);
		else if (!strnicmp(ext, "PID=", 4)) track_id = atoi(&ext[4]);
		else if (!strnicmp(ext, "program=", 8)) {
			for (i=0; i<import.nb_progs; i++) {
				if (!stricmp(import.pg_info[i].name, ext+8)) {
					prog_id = import.pg_info[i].number;
					do_all = 0;
					break;
				}
			}
		}
		else if (!strnicmp(ext, "prog_id=", 8)) {
			prog_id = atoi(ext+8);
			do_all = 0;
		}
		else track_id = atoi(ext);

		//figure out trackID
		if (do_audio || do_video || do_auxv || do_pict || track_id) {
			Bool found = track_id ? GF_FALSE : GF_TRUE;
			for (i=0; i<import.nb_tracks; i++) {
				if (track_id && (import.tk_info[i].track_num==track_id)) {
					found=GF_TRUE;
					break;
				}
				if (do_audio && (import.tk_info[i].stream_type==GF_STREAM_AUDIO)) {
					track_id = import.tk_info[i].track_num;
					break;
				}
				else if (do_video && (import.tk_info[i].stream_type==GF_STREAM_VISUAL)) {
					track_id = import.tk_info[i].track_num;
					break;
				}
				else if (do_auxv && (import.tk_info[i].media_subtype==GF_ISOM_MEDIA_AUXV)) {
					track_id = import.tk_info[i].track_num;
					break;
				}
				else if (do_pict && (import.tk_info[i].media_subtype==GF_ISOM_MEDIA_PICT)) {
					track_id = import.tk_info[i].track_num;
					break;
				}
			}
			if (!track_id || !found) {
				M4_LOG(GF_LOG_ERROR, ("Cannot find track ID matching fragment #%s\n", ext));
				if (sep) sep[0] = ':';
				e = GF_NOT_FOUND;
				goto exit;
			}
		}
		if (sep) sep[0] = ':';
	}
	if (do_audio || do_video || do_auxv || do_pict || track_id) do_all = 0;

	if (track_layout || is_chap) {
		u32 w, h, sw, sh, fw, fh;
		w = h = sw = sh = fw = fh = 0;
		chap_ref = 0;
		for (i=0; i<gf_isom_get_track_count(dest); i++) {
			switch (gf_isom_get_media_type(dest, i+1)) {
			case GF_ISOM_MEDIA_SCENE:
			case GF_ISOM_MEDIA_VISUAL:
            case GF_ISOM_MEDIA_AUXV:
            case GF_ISOM_MEDIA_PICT:
				if (!chap_ref && gf_isom_is_track_enabled(dest, i+1) ) chap_ref = i+1;

				gf_isom_get_visual_info(dest, i+1, 1, &sw, &sh);
				gf_isom_get_track_layout_info(dest, i+1, &fw, &fh, NULL, NULL, NULL);
				if (w<sw) w = sw;
				if (w<fw) w = fw;
				if (h<sh) h = sh;
				if (h<fh) h = fh;
				break;
			case GF_ISOM_MEDIA_AUDIO:
				if (!chap_ref && gf_isom_is_track_enabled(dest, i+1) ) chap_ref = i+1;
				break;
			}
		}
		if (track_layout) {
			if (!tw) tw = w;
			if (!th) th = h;
			if (ty==-1) ty = (h>(u32)th) ? h-th : 0;
			import.text_width = tw;
			import.text_height = th;
		}
		if (is_chap && chap_ref) import_flags |= GF_IMPORT_NO_TEXT_FLUSH;
	}
	if (text_layout && txtw && txth) {
		import.text_track_width = import.text_width ? import.text_width : txtw;
		import.text_track_height = import.text_height ? import.text_height : txth;
		import.text_width = txtw;
		import.text_height = txth;
		import.text_x = txtx;
		import.text_y = txty;
	}

	check_track_for_svc = check_track_for_lhvc = check_track_for_hevc = 0;

	source_magic = (u64) gf_crc_32((u8 *)inName, (u32) strlen(inName));
	if (!fake_import && (!fsess || mux_args_if_first_pass)) {
		import.in_name = szName;
		import.dest = dest;
		import.video_fps = force_fps;
		import.frames_per_sample = frames_per_sample;
		import.flags = import_flags;
		import.keep_audelim = keep_audelim;
		import.print_stats_graph = print_stats_graph;
		import.xps_inband = xps_inband;
		import.prog_id = prog_id;
		import.trackID = track_id;
		import.source_magic = source_magic;
		import.track_index = tk_idx;

		//if moov timescale is <0 (auto mode) set it at import time
		if (moov_timescale<0) {
			import.moov_timescale = moov_timescale;
		}
		//otherwise force it now
		else if (moov_timescale>0) {
			e = gf_isom_set_timescale(dest, moov_timescale);
			GOTO_EXIT("changing timescale")
		}

		import.run_in_session = fsess;
		import.update_mux_args = NULL;
		if (do_all)
			import.flags |= GF_IMPORT_KEEP_REFS;

		e = gf_media_import(&import);
		if (e) {
			if (import.update_mux_args) gf_free(import.update_mux_args);
			GOTO_EXIT("importing media");
		}

		if (fsess) {
			*mux_args_if_first_pass = import.update_mux_args;
			import.update_mux_args = NULL;
			goto exit;
		}
	}

	nb_tracks = gf_isom_get_track_count(dest);
	for (i=0; i<nb_tracks; i++) {
		u32 media_type;
		track = i+1;
		media_type = gf_isom_get_media_type(dest, track);
		e = GF_OK;
		if (!fake_import) {
			u64 tk_source_magic;
			tk_source_magic = gf_isom_get_track_magic(dest, track);

			if ((tk_source_magic & 0xFFFFFFFFUL) != source_magic)
				continue;
			tk_source_magic>>=32;		
			keep_handler = (tk_source_magic & 1) ? GF_TRUE : GF_FALSE;
		} else {
			keep_handler = GF_TRUE;

			if (do_audio && (media_type!=GF_ISOM_MEDIA_AUDIO)) continue;
			if (do_video && (media_type!=GF_ISOM_MEDIA_VISUAL)) continue;
			if (do_auxv && (media_type!=GF_ISOM_MEDIA_AUXV)) continue;
			if (do_pict && (media_type!=GF_ISOM_MEDIA_PICT)) continue;
			if (track_id && (gf_isom_get_track_id(dest, track) != track_id))
				continue;
		}

		timescale = gf_isom_get_timescale(dest);
		if (szLan) {
			e = gf_isom_set_media_language(dest, track, (char *) szLan);
			GOTO_EXIT("changing language")
		}
		if (do_disable) {
			e = gf_isom_set_track_enabled(dest, track, (do_disable==2) ? GF_TRUE : GF_FALSE);
			GOTO_EXIT("disabling track")
		}
		if (track_flags_mode) {
			e = gf_isom_set_track_flags(dest, track, track_flags, track_flags_mode);
			GOTO_EXIT("disabling track")
		}

		if (import_flags & GF_IMPORT_NO_EDIT_LIST) {
			e = gf_isom_remove_edits(dest, track);
			GOTO_EXIT("removing edits")
		}
		if (delay.num && delay.den) {
			u64 tk_dur;
			e = gf_isom_remove_edits(dest, track);
			tk_dur = gf_isom_get_track_duration(dest, track);
			if (delay.num>0) {
				//cast to s64, timescale*delay could be quite large before /1000
				e |= gf_isom_append_edit(dest, track, ((s64) delay.num) * timescale / delay.den, 0, GF_ISOM_EDIT_EMPTY);
				e |= gf_isom_append_edit(dest, track, tk_dur, 0, GF_ISOM_EDIT_NORMAL);
			} else {
					//cast to s64, timescale*delay could be quite large before /1000
				u64 to_skip = ((s64) -delay.num) * timescale / delay.den;
				if (to_skip<tk_dur) {
					//cast to s64, timescale*delay could be quite large before /1000
					u64 media_time = ((s64) -delay.num) * gf_isom_get_media_timescale(dest, track) / delay.den;
					e |= gf_isom_append_edit(dest, track, tk_dur-to_skip, media_time, GF_ISOM_EDIT_NORMAL);
				} else {
					M4_LOG(GF_LOG_ERROR, ("Warning: request negative delay longer than track duration - ignoring\n"));
				}
			}
			GOTO_EXIT("assigning delay")
		}
		if (gf_isom_is_video_handler_type(media_type)) {
			if (((par_n>=0) && (par_d>=0)) || force_par) {
				e = gf_media_change_par(dest, track, par_n, par_d, force_par, rewrite_bs);
				GOTO_EXIT("changing PAR")
			}
			if ((fullrange>=0) || (videofmt>=0) || (colorprim>=0) || (colortfc>=0) || (colormx>=0)) {
				e = gf_media_change_color(dest, i+1, fullrange, videofmt, colorprim, colortfc, colormx);
				GOTO_EXIT("changing color in bitstream")
			}
			if (has_clap) {
				e = gf_isom_set_clean_aperture(dest, track, 1, clap_wn, clap_wd, clap_hn, clap_hd, clap_hon, clap_hod, clap_von, clap_vod);
				GOTO_EXIT("changing clean aperture")
			}
			if (bitdepth) {
				e = gf_isom_set_visual_bit_depth(dest, track, 1, bitdepth);
				GOTO_EXIT("changing bit depth")
			}
			if (clr_type) {
				e = gf_isom_set_visual_color_info(dest, track, 1, clr_type, clr_prim, clr_tranf, clr_mx, clr_full_range, icc_data, icc_size);
				GOTO_EXIT("changing color info")
			}
			if (dv_profile) {
				e = gf_isom_set_dolby_vision_profile(dest, track, 1, dv_profile);
				GOTO_EXIT("setting DV profile")
			}

			if (set_ccst) {
				e = gf_isom_set_image_sequence_coding_constraints(dest, track, 1, GF_FALSE, GF_FALSE, GF_TRUE, 15);
				GOTO_EXIT("setting image sequence constraints")
			}
		}
		if (has_mx) {
			e = gf_isom_set_track_matrix(dest, track, mx);
			GOTO_EXIT("setting track matrix")
		}
		if (use_stz2) {
			e = gf_isom_use_compact_size(dest, track, GF_TRUE);
			GOTO_EXIT("setting compact size")
		}

		if (gf_isom_get_media_subtype(dest, track, 1) == GF_ISOM_MEDIA_TIMECODE) {
			tmcd_track = track;
		}
		if (rap_only || refs_only) {
			e = gf_media_remove_non_rap(dest, track, refs_only);
			GOTO_EXIT("removing non RAPs")
		}
		if (handler_name) {
			e = gf_isom_set_handler_name(dest, track, handler_name);
			GOTO_EXIT("setting handler name")
		}
		else if (!keep_handler) {
			char szHName[1024];
			const char *fName = gf_url_get_resource_name((const  char *)inName);
			fName = strchr(fName, '.');
			if (fName) fName += 1;
			else fName = "?";

			sprintf(szHName, "%s@GPAC%s", fName, gf_gpac_version());
			e = gf_isom_set_handler_name(dest, track, szHName);
			GOTO_EXIT("setting handler name")
		}
		if (handler) {
			e = gf_isom_set_media_type(dest, track, handler);
			GOTO_EXIT("setting media type")
		}
		if (group) {
			e = gf_isom_set_alternate_group_id(dest, track, group);
			GOTO_EXIT("setting alternate group")
		}

		if (track_layout) {
			e = gf_isom_set_track_layout_info(dest, track, tw<<16, th<<16, tx<<16, ty<<16, tz);
			GOTO_EXIT("setting track layout")
		}
		if (stype) {
			e = gf_isom_set_media_subtype(dest, track, 1, stype);
			GOTO_EXIT("setting media subtype")
		}
		if (is_chap && chap_ref) {
			e = set_chapter_track(dest, track, chap_ref);
			GOTO_EXIT("setting chapter track")
		}

		for (j = 0; j < gf_list_count(kinds); j+=2) {
			char *kind_scheme = (char *)gf_list_get(kinds, j);
			char *kind_value = (char *)gf_list_get(kinds, j+1);
			e = gf_isom_add_track_kind(dest, i+1, kind_scheme, kind_value);
			GOTO_EXIT("setting track kind")
		}

		if (profile || compat || level) {
			e = gf_media_change_pl(dest, track, profile, compat, level);
			GOTO_EXIT("changing video PL")
		}
		if (gf_isom_get_mpeg4_subtype(dest, track, 1))
			keep_sys_tracks = 1;

		//if moov timescale is <0 (auto mode) set it at import time
		if (fake_import) {
			if (import_flags & GF_IMPORT_NO_EDIT_LIST)
				gf_isom_remove_edits(dest, track);

			if (moov_timescale<0) {
				moov_timescale = gf_isom_get_media_timescale(dest, track);
			}
			if (moov_timescale>0) {
				e = gf_isom_set_timescale(dest, moov_timescale);
				GOTO_EXIT("changing timescale")
			}

			if (import.asemode && (media_type==GF_ISOM_MEDIA_AUDIO)) {
				u32 sr, ch, bps;
				gf_isom_get_audio_info(dest, track, 1, &sr, &ch, &bps);
				gf_isom_set_audio_info(dest, track, 1, sr, ch, bps, import.asemode);
			}
		}

		if (roll_change) {
			if ((roll_change!=3) || (media_type==GF_ISOM_MEDIA_AUDIO)) {
				e = gf_isom_set_sample_roll_group(dest, track, (u32) -1, (roll_change==2) ? GF_ISOM_SAMPLE_PREROLL : GF_ISOM_SAMPLE_ROLL, roll);
				GOTO_EXIT("assigning roll")
			}
		}

		if (new_timescale>1) {
			e = gf_isom_set_media_timescale(dest, track, new_timescale, 0, 0);
			GOTO_EXIT("setting media timescale")
		}

		if (rescale_num > 1) {
			switch (gf_isom_get_media_type(dest, track)) {
			case GF_ISOM_MEDIA_AUDIO:
				if (!rescale_override) {
					M4_LOG(GF_LOG_WARNING, ("Cannot force media timescale for audio media types - ignoring\n"));
					break;
				}
			default:
				e = gf_isom_set_media_timescale(dest, track, rescale_num, rescale_den, rescale_override ? 2 : 1);
                if (e==GF_EOS) {
					M4_LOG(GF_LOG_WARNING, ("Rescale ignored, same config in source file\n"));
					e = GF_OK;
				}
				GOTO_EXIT("rescaling media track")
				break;
			}
		}

		if (has_last_sample_dur) {
			e = gf_isom_set_last_sample_duration_ex(dest, track, last_sample_dur.num, last_sample_dur.den);
			GOTO_EXIT("setting last sample duration")
		}
		if (rvc_config) {
#ifdef GPAC_DISABLE_ZLIB
			M4_LOG(GF_LOG_ERROR, ("Error: no zlib support - RVC not available\n"));
			e = GF_NOT_SUPPORTED;
			goto exit;
#else
			u8 *data;
			u32 size;
			e = gf_file_load_data(rvc_config, (u8 **) &data, &size);
			GOTO_EXIT("loading RVC config file")

			gf_gz_compress_payload(&data, size, &size);
			e |= gf_isom_set_rvc_config(dest, track, 1, 0, "application/rvc-config+xml+gz", data, size);
			gf_free(data);
			GOTO_EXIT("compressing and assigning RVC config")
#endif
		} else if (rvc_predefined>0) {
			e = gf_isom_set_rvc_config(dest, track, 1, rvc_predefined, NULL, NULL, 0);
			GOTO_EXIT("setting RVC predefined config")
		}

		if (neg_ctts_mode) {
			e = gf_isom_set_composition_offset_mode(dest, track, (neg_ctts_mode==1) ? GF_TRUE : GF_FALSE);
			GOTO_EXIT("setting composition offset mode")
		}

		if (gf_isom_get_avc_svc_type(dest, track, 1)>=GF_ISOM_AVCTYPE_AVC_SVC)
			check_track_for_svc = track;

		switch (gf_isom_get_hevc_lhvc_type(dest, track, 1)) {
		case GF_ISOM_HEVCTYPE_HEVC_LHVC:
		case GF_ISOM_HEVCTYPE_LHVC_ONLY:
			check_track_for_lhvc = i+1;
			break;
		case GF_ISOM_HEVCTYPE_HEVC_ONLY:
			check_track_for_hevc=1;
			break;
		default:
			break;
		}

		if (txt_flags) {
			e = gf_isom_text_set_display_flags(dest, track, 0, txt_flags, txt_mode);
			GOTO_EXIT("setting text track display flags")
		}

		if (edits) {
			e = apply_edits(dest, track, edits);
			GOTO_EXIT("applying edits")
		}

		if (force_rate>=0) {
			e = gf_isom_update_bitrate(dest, i+1, 1, force_rate, force_rate, 0);
			GOTO_EXIT("updating bitrate")
		}

		if (split_tile_mode) {
			switch (gf_isom_get_media_subtype(dest, track, 1)) {
			case GF_ISOM_SUBTYPE_HVC1:
			case GF_ISOM_SUBTYPE_HEV1:
			case GF_ISOM_SUBTYPE_HVC2:
			case GF_ISOM_SUBTYPE_HEV2:
				break;
			default:
				split_tile_mode = 0;
				break;
			}
		}
	}

	if (chapter_name) {
		if (is_chap_file) {
			GF_Fraction a_fps = {0,0};
			e = gf_media_import_chapters(dest, chapter_name, a_fps, GF_FALSE);
		} else {
			e = gf_isom_add_chapter(dest, 0, 0, chapter_name);
		}
		GOTO_EXIT("importing chapters")
	}

	if (tmcd_track) {
		u32 tmcd_id = gf_isom_get_track_id(dest, tmcd_track);
		for (i=0; i < gf_isom_get_track_count(dest); i++) {
			switch (gf_isom_get_media_type(dest, i+1)) {
			case GF_ISOM_MEDIA_VISUAL:
			case GF_ISOM_MEDIA_AUXV:
			case GF_ISOM_MEDIA_PICT:
				break;
			default:
				continue;
			}
			e = gf_isom_set_track_reference(dest, i+1, GF_ISOM_REF_TMCD, tmcd_id);
			GOTO_EXIT("assigning TMCD track references")
		}
	}

	/*force to rewrite all dependencies*/
	for (i = 1; i <= gf_isom_get_track_count(dest); i++)
	{
		e = gf_isom_rewrite_track_dependencies(dest, i);
		if (e) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_CONTAINER, ("Warning: track ID %d has references to a track not imported\n", gf_isom_get_track_id(dest, i) ));
			e = GF_OK;
		}
	}

#ifndef GPAC_DISABLE_AV_PARSERS
	if (max_layer_id_plus_one || max_temporal_id_plus_one) {
		for (i = 1; i <= gf_isom_get_track_count(dest); i++)
		{
			e = gf_media_filter_hevc(dest, i, max_temporal_id_plus_one, max_layer_id_plus_one);
			if (e) {
				M4_LOG(GF_LOG_ERROR, ("Warning: track ID %d: error while filtering LHVC layers\n", gf_isom_get_track_id(dest, i)));
				e = GF_OK;
			}
		}
	}
#endif

	if (check_track_for_svc) {
		if (svc_mode) {
			e = gf_media_split_svc(dest, check_track_for_svc, (svc_mode==2) ? 1 : 0);
			GOTO_EXIT("splitting SVC track")
		} else {
			e = gf_media_merge_svc(dest, check_track_for_svc, 1);
			GOTO_EXIT("merging SVC/SHVC track")
		}
	}
#ifndef GPAC_DISABLE_AV_PARSERS
	if (check_track_for_lhvc) {
		if (svc_mode) {
			GF_LHVCExtractoreMode xmode = GF_LHVC_EXTRACTORS_ON;
			if (svc_mode==3) xmode = GF_LHVC_EXTRACTORS_OFF;
			else if (svc_mode==4) xmode = GF_LHVC_EXTRACTORS_OFF_FORCE_INBAND;
			e = gf_media_split_lhvc(dest, check_track_for_lhvc, GF_FALSE, (svc_mode==1) ? 0 : 1, xmode );
			GOTO_EXIT("splitting L-HEVC track")
		} else {
			//TODO - merge, temporal sublayers
		}
	}
#ifndef GPAC_DISABLE_HEVC
	if (check_track_for_hevc) {
		if (split_tile_mode) {
			e = gf_media_split_hevc_tiles(dest, split_tile_mode - 1);
			GOTO_EXIT("splitting HEVC tiles")
		}
		if (temporal_mode) {
			GF_LHVCExtractoreMode xmode = (temporal_mode==3) ? GF_LHVC_EXTRACTORS_OFF : GF_LHVC_EXTRACTORS_ON;
			e = gf_media_split_lhvc(dest, check_track_for_hevc, GF_TRUE, (temporal_mode==1) ? GF_FALSE : GF_TRUE, xmode );
			GOTO_EXIT("splitting HEVC temporal sublayers")
		}
	}
#endif

	if (tc_fps_num) {
		u32 desc_index=0;
		u32 tmcd_tk, tmcd_id;
		u32 video_ref = 0;
		GF_BitStream *bs;
		GF_ISOSample *samp;
		for (i=0; i<gf_isom_get_track_count(dest); i++) {
			if (gf_isom_is_video_handler_type(gf_isom_get_media_type(dest, i+1))) {
				video_ref = i+1;
				break;
			}
		}
		tmcd_tk = gf_isom_new_track(dest, 0, GF_ISOM_MEDIA_TIMECODE, tc_fps_num);
		if (!tmcd_tk) {
			e = gf_isom_last_error(dest);
			GOTO_EXIT("creating TMCD track")
		}
		e = gf_isom_set_track_enabled(dest, tmcd_tk, 1);
		if (e != GF_OK) {
			GOTO_EXIT("enabling TMCD track")
		}

		if (!tc_frames_per_tick) {
			tc_frames_per_tick = tc_fps_num;
			tc_frames_per_tick /= tc_fps_den;
			if (tc_frames_per_tick * tc_fps_den < tc_fps_num)
				tc_frames_per_tick++;
		}

		u32 tmcd_value = (tc_h * 3600 + tc_m*60 + tc_s)*tc_frames_per_tick+tc_f;
		tmcd_id = gf_isom_get_track_id(dest, tmcd_tk);

		e = gf_isom_tmcd_config_new(dest, tmcd_tk, tc_fps_num, tc_fps_den, tc_frames_per_tick, tc_drop_frame, tc_force_counter, &desc_index);
		GOTO_EXIT("configuring TMCD sample description")

		if (video_ref) {
			e = gf_isom_set_track_reference(dest, video_ref, GF_ISOM_REF_TMCD, tmcd_id);
			GOTO_EXIT("assigning TMCD track ref on video track")
		}
		bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
		gf_bs_write_u32(bs, tmcd_value);
		samp = gf_isom_sample_new();
		samp->IsRAP = SAP_TYPE_1;
		gf_bs_get_content(bs, &samp->data, &samp->dataLength);
		gf_bs_del(bs);
		e = gf_isom_add_sample(dest, tmcd_tk, desc_index, samp);
		gf_isom_sample_del(&samp);
		GOTO_EXIT("assigning TMCD sample")

		if (video_ref) {
			u64 video_ref_dur = gf_isom_get_media_duration(dest, video_ref);
			video_ref_dur *= tc_fps_num;
			video_ref_dur /= gf_isom_get_media_timescale(dest, video_ref);
			e = gf_isom_set_last_sample_duration(dest, tmcd_tk, (u32) video_ref_dur);
		} else {
			e = gf_isom_set_last_sample_duration(dest, tmcd_tk, tc_fps_den ? tc_fps_den : 1);
		}
		GOTO_EXIT("setting TMCD sample dur")
	}

#endif /*GPAC_DISABLE_AV_PARSERS*/

exit:
	while (gf_list_count(kinds)) {
		char *kind = (char *)gf_list_get(kinds, 0);
		gf_list_rem(kinds, 0);
		if (kind) gf_free(kind);
	}
	if (opt_src) opt_src[0] = ':';
	if (opt_dst) opt_dst[0] = ':';
	if (fchain) fchain[0] = ':';

	gf_list_del(kinds);
	if (handler_name) gf_free(handler_name);
	if (chapter_name ) gf_free(chapter_name);
	if (import.fontName) gf_free(import.fontName);
	if (import.streamFormat) gf_free(import.streamFormat);
	if (import.force_ext) gf_free(import.force_ext);
	if (rvc_config) gf_free(rvc_config);
	if (edits) gf_free(edits);
	if (szLan) gf_free((char *)szLan);
	if (icc_data) gf_free(icc_data);

	if (!e) return GF_OK;
	if (fail_msg) {
		M4_LOG(GF_LOG_ERROR, ("Failure while %s: %s\n", fail_msg, gf_error_to_string(e) ));
	}
	return e;
}