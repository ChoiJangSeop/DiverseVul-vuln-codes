GF_RTPHinter *gf_hinter_track_new(GF_ISOFile *file, u32 TrackNum,
                                  u32 Path_MTU, u32 max_ptime, u32 default_rtp_rate, u32 flags, u8 PayloadID,
                                  Bool copy_media, u32 InterleaveGroupID, u8 InterleaveGroupPriority, GF_Err *e)
{

	GF_SLConfig my_sl;
	u32 descIndex, MinSize, MaxSize, avgTS, streamType, codecid, const_dur, nb_ch, maxDTSDelta;
	u8 OfficialPayloadID;
	u32 TrackMediaSubType, TrackMediaType, hintType, nbEdts, required_rate, force_dts_delta, avc_nalu_size, PL_ID, bandwidth, IV_length, KI_length;
	const char *url, *urn;
	char *mpeg4mode;
	Bool is_crypted, has_mpeg4_mapping;
	GF_RTPHinter *tmp;
	GF_ESD *esd;

	*e = GF_BAD_PARAM;
	if (!file || !TrackNum || !gf_isom_get_track_id(file, TrackNum)) return NULL;

	if (!gf_isom_get_sample_count(file, TrackNum)) {
		*e = GF_OK;
		return NULL;
	}
	*e = GF_NOT_SUPPORTED;
	nbEdts = gf_isom_get_edits_count(file, TrackNum);
	if (nbEdts>1) {
		u64 et, sd, mt;
		GF_ISOEditType em;
		gf_isom_get_edit(file, TrackNum, 1, &et, &sd, &mt, &em);
		if ((nbEdts>2) || (em!=GF_ISOM_EDIT_EMPTY)) {
			GF_LOG(GF_LOG_ERROR, GF_LOG_RTP, ("[rtp hinter] Cannot hint track whith EditList\n"));
			return NULL;
		}
	}
	if (nbEdts) gf_isom_remove_edits(file, TrackNum);

	if (!gf_isom_is_track_enabled(file, TrackNum)) return NULL;

	/*by default NO PL signaled*/
	PL_ID = 0;
	OfficialPayloadID = 0;
	force_dts_delta = 0;
	streamType = 0;
	mpeg4mode = NULL;
	required_rate = 0;
	is_crypted = 0;
	IV_length = KI_length = 0;
	codecid = 0;
	nb_ch = 0;
	avc_nalu_size = 0;
	has_mpeg4_mapping = 1;
	const_dur = 0;
	bandwidth=0;
	TrackMediaType = gf_isom_get_media_type(file, TrackNum);

	/*for max compatibility with QT*/
	if (!default_rtp_rate) default_rtp_rate = 90000;

	/*timed-text is a bit special, we support multiple stream descriptions & co*/
	if ( (TrackMediaType==GF_ISOM_MEDIA_TEXT) || (TrackMediaType==GF_ISOM_MEDIA_SUBT)) {
		hintType = GF_RTP_PAYT_3GPP_TEXT;
		codecid = GF_CODECID_TEXT_MPEG4;
		streamType = GF_STREAM_TEXT;
		/*fixme - this works cos there's only one PL for text in mpeg4 at the current time*/
		PL_ID = 0x10;
	} else {
		if (gf_isom_get_sample_description_count(file, TrackNum) > 1) return NULL;

		TrackMediaSubType = gf_isom_get_media_subtype(file, TrackNum, 1);
		switch (TrackMediaSubType) {
		case GF_ISOM_SUBTYPE_MPEG4_CRYP:
			is_crypted = 1;
		case GF_ISOM_SUBTYPE_MPEG4:
			esd = gf_isom_get_esd(file, TrackNum, 1);
			hintType = GF_RTP_PAYT_MPEG4;
			if (esd && esd->decoderConfig) {
				streamType = esd->decoderConfig->streamType;
				codecid = esd->decoderConfig->objectTypeIndication;
				if (esd->URLString) hintType = 0;
				/*AAC*/
				if ((streamType==GF_STREAM_AUDIO)
					&& esd->decoderConfig->decoderSpecificInfo && esd->decoderConfig->decoderSpecificInfo->data
				        /*(nb: we use mpeg4 for MPEG-2 AAC)*/
				        && ((codecid==GF_CODECID_AAC_MPEG4) || (codecid==GF_CODECID_AAC_MPEG2_MP) || (codecid==GF_CODECID_AAC_MPEG2_LCP) || (codecid==GF_CODECID_AAC_MPEG2_SSRP)) ) {

					u32 sample_rate;
					GF_M4ADecSpecInfo a_cfg;
					gf_m4a_get_config(esd->decoderConfig->decoderSpecificInfo->data, esd->decoderConfig->decoderSpecificInfo->dataLength, &a_cfg);
					nb_ch = a_cfg.nb_chan;
					sample_rate = a_cfg.base_sr;
					PL_ID = a_cfg.audioPL;
					switch (a_cfg.base_object_type) {
					case GF_M4A_AAC_MAIN:
					case GF_M4A_AAC_LC:
						if (flags & GP_RTP_PCK_USE_LATM_AAC) {
							hintType = GF_RTP_PAYT_LATM;
							break;
						}
					case GF_M4A_AAC_SBR:
					case GF_M4A_AAC_PS:
					case GF_M4A_AAC_LTP:
					case GF_M4A_AAC_SCALABLE:
					case GF_M4A_ER_AAC_LC:
					case GF_M4A_ER_AAC_LTP:
					case GF_M4A_ER_AAC_SCALABLE:
						mpeg4mode = "AAC";
						break;
					case GF_M4A_CELP:
					case GF_M4A_ER_CELP:
						mpeg4mode = "CELP";
						break;
					}
					required_rate = sample_rate;
				}
				/*MPEG1/2 audio*/
				else if ((streamType==GF_STREAM_AUDIO) && ((codecid==GF_CODECID_MPEG2_PART3) || (codecid==GF_CODECID_MPEG_AUDIO))) {
					GF_ISOSample *samp = NULL;
					if (!is_crypted)
						 samp = gf_isom_get_sample(file, TrackNum, 1, NULL);

					if (samp && (samp->dataLength>3)) {
						u32 hdr = GF_4CC((u32)samp->data[0], (u8)samp->data[1], (u8)samp->data[2], (u8)samp->data[3]);
						nb_ch = gf_mp3_num_channels(hdr);
						hintType = GF_RTP_PAYT_MPEG12_AUDIO;
						/*use official RTP/AVP payload type*/
						OfficialPayloadID = 14;
						required_rate = 90000;
					}
					/*encrypted MP3 must be sent through MPEG-4 generic to signal all ISMACryp stuff*/
					else {
						u32 sample_rate;
						gf_isom_get_audio_info(file, TrackNum, 1, &sample_rate, &nb_ch, NULL);
						required_rate = sample_rate;
					}
					if (samp)
						gf_isom_sample_del(&samp);

				}
				/*QCELP audio*/
				else if ((streamType==GF_STREAM_AUDIO) && (codecid==GF_CODECID_QCELP)) {
					hintType = GF_RTP_PAYT_QCELP;
					OfficialPayloadID = 12;
					required_rate = 8000;
					streamType = GF_STREAM_AUDIO;
					nb_ch = 1;
				}
				/*EVRC/SVM audio*/
				else if ((streamType==GF_STREAM_AUDIO) && ((codecid==GF_CODECID_EVRC) || (codecid==GF_CODECID_SMV)) ) {
					hintType = GF_RTP_PAYT_EVRC_SMV;
					required_rate = 8000;
					streamType = GF_STREAM_AUDIO;
					nb_ch = 1;
				}
				/*visual streams*/
				else if (streamType==GF_STREAM_VISUAL) {
					if ((codecid==GF_CODECID_MPEG4_PART2) && esd->decoderConfig->decoderSpecificInfo) {
						GF_M4VDecSpecInfo dsi;
						gf_m4v_get_config(esd->decoderConfig->decoderSpecificInfo->data, esd->decoderConfig->decoderSpecificInfo->dataLength, &dsi);
						PL_ID = dsi.VideoPL;
					}
					/*MPEG1/2 video*/
					if ( ((codecid>=GF_CODECID_MPEG2_SIMPLE) && (codecid<=GF_CODECID_MPEG2_422)) || (codecid==GF_CODECID_MPEG1)) {
						if (!is_crypted) {
							hintType = GF_RTP_PAYT_MPEG12_VIDEO;
							OfficialPayloadID = 32;
						}
					}
					/*for ISMA*/
					if (is_crypted) {
						/*that's another pain with ISMACryp, even if no B-frames the DTS is signaled...*/
						if (codecid==GF_CODECID_MPEG4_PART2) force_dts_delta = 22;
						else if ((codecid==GF_CODECID_AVC) || (codecid==GF_CODECID_SVC)) {
							flags &= ~GP_RTP_PCK_USE_MULTI;
							force_dts_delta = 22;
						}
						flags |= GP_RTP_PCK_SIGNAL_RAP | GP_RTP_PCK_SIGNAL_TS;
					}

					required_rate = default_rtp_rate;
				}
				/*systems streams*/
				else if (gf_isom_has_sync_shadows(file, TrackNum) || gf_isom_has_sample_dependency(file, TrackNum)) {
					flags |= GP_RTP_PCK_SYSTEMS_CAROUSEL;
				}
				gf_odf_desc_del((GF_Descriptor*)esd);
			}
			break;
		case GF_ISOM_SUBTYPE_3GP_H263:
			hintType = GF_RTP_PAYT_H263;
			required_rate = 90000;
			streamType = GF_STREAM_VISUAL;
			OfficialPayloadID = 34;
			/*not 100% compliant (short header is missing) but should still work*/
			codecid = GF_CODECID_MPEG4_PART2;
			PL_ID = 0x01;
			break;
		case GF_ISOM_SUBTYPE_3GP_AMR:
			required_rate = 8000;
			hintType = GF_RTP_PAYT_AMR;
			streamType = GF_STREAM_AUDIO;
			has_mpeg4_mapping = 0;
			nb_ch = 1;
			break;
		case GF_ISOM_SUBTYPE_3GP_AMR_WB:
			required_rate = 16000;
			hintType = GF_RTP_PAYT_AMR_WB;
			streamType = GF_STREAM_AUDIO;
			has_mpeg4_mapping = 0;
			nb_ch = 1;
			break;
		case GF_ISOM_SUBTYPE_AVC_H264:
		case GF_ISOM_SUBTYPE_AVC2_H264:
		case GF_ISOM_SUBTYPE_AVC3_H264:
		case GF_ISOM_SUBTYPE_AVC4_H264:
		case GF_ISOM_SUBTYPE_SVC_H264:
		case GF_ISOM_SUBTYPE_MVC_H264:
		{
			GF_AVCConfig *avcc = gf_isom_avc_config_get(file, TrackNum, 1);
			GF_AVCConfig *svcc = gf_isom_svc_config_get(file, TrackNum, 1);
			GF_AVCConfig *mvcc = gf_isom_mvc_config_get(file, TrackNum, 1);

			if (!avcc && !svcc && !mvcc) {
				*e = GF_NON_COMPLIANT_BITSTREAM;
				return NULL;
			}

			required_rate = 90000;	/* "90 kHz clock rate MUST be used"*/
			hintType = GF_RTP_PAYT_H264_AVC;
			if (TrackMediaSubType==GF_ISOM_SUBTYPE_SVC_H264)
				hintType = GF_RTP_PAYT_H264_SVC;
			else if (TrackMediaSubType==GF_ISOM_SUBTYPE_MVC_H264)
				hintType = GF_RTP_PAYT_H264_SVC;
			streamType = GF_STREAM_VISUAL;
			avc_nalu_size = avcc ? avcc->nal_unit_size : svcc ? svcc->nal_unit_size : mvcc->nal_unit_size;
			codecid = GF_CODECID_AVC;
			PL_ID = 0x0F;
			gf_odf_avc_cfg_del(avcc);
			gf_odf_avc_cfg_del(svcc);
		}
		break;
		case GF_ISOM_SUBTYPE_HVC1:
		case GF_ISOM_SUBTYPE_HEV1:
		case GF_ISOM_SUBTYPE_HVC2:
		case GF_ISOM_SUBTYPE_HEV2:
		{
			GF_HEVCConfig *hevcc = gf_isom_hevc_config_get(file, TrackNum, 1);
			if (!hevcc) {
				*e = GF_NON_COMPLIANT_BITSTREAM;
				return NULL;
			}
			required_rate = 90000;	/* "90 kHz clock rate MUST be used"*/
			hintType = GF_RTP_PAYT_HEVC;
			streamType = GF_STREAM_VISUAL;
			avc_nalu_size = hevcc->nal_unit_size;
			codecid = GF_CODECID_HEVC;
			PL_ID = 0x0F;
			flags |= GP_RTP_PCK_USE_MULTI;
			gf_odf_hevc_cfg_del(hevcc);
			break;
		}
		break;
		case GF_ISOM_SUBTYPE_3GP_QCELP:
			required_rate = 8000;
			hintType = GF_RTP_PAYT_QCELP;
			streamType = GF_STREAM_AUDIO;
			codecid = GF_CODECID_QCELP;
			OfficialPayloadID = 12;
			nb_ch = 1;
			break;
		case GF_ISOM_SUBTYPE_3GP_EVRC:
		case GF_ISOM_SUBTYPE_3GP_SMV:
			required_rate = 8000;
			hintType = GF_RTP_PAYT_EVRC_SMV;
			streamType = GF_STREAM_AUDIO;
			codecid = (TrackMediaSubType==GF_ISOM_SUBTYPE_3GP_EVRC) ? GF_CODECID_EVRC : GF_CODECID_SMV;
			nb_ch = 1;
			break;
		case GF_ISOM_SUBTYPE_3GP_DIMS:
#if GPAC_ENABLE_3GPP_DIMS_RTP
			hintType = GF_RTP_PAYT_3GPP_DIMS;
			streamType = GF_STREAM_SCENE;
#else
			hintType = 0;
			GF_LOG(GF_LOG_ERROR, GF_LOG_RTP, ("[RTP Packetizer] 3GPP DIMS over RTP disabled in build\n", streamType));
#endif
			break;
		case GF_ISOM_SUBTYPE_AC3:
			hintType = GF_RTP_PAYT_AC3;
			streamType = GF_STREAM_AUDIO;
			gf_isom_get_audio_info(file, TrackNum, 1, NULL, &nb_ch, NULL);
			break;
		case GF_ISOM_SUBTYPE_MP3:
		{
			GF_ISOSample *samp = gf_isom_get_sample(file, TrackNum, 1, NULL);
			if (samp && (samp->dataLength>3)) {
				u32 hdr = GF_4CC((u32)samp->data[0], (u8)samp->data[1], (u8)samp->data[2], (u8)samp->data[3]);
				nb_ch = gf_mp3_num_channels(hdr);
			} else {
				u32 bps;
				gf_isom_get_audio_info(file, TrackNum, 1, &required_rate, &nb_ch, &bps);
			}
			hintType = GF_RTP_PAYT_MPEG12_AUDIO;
			/*use official RTP/AVP payload type*/
			OfficialPayloadID = 14;
			required_rate = 90000;

			if (samp)
				gf_isom_sample_del(&samp);
		}
		break;
		default:
			/*ERROR*/
			hintType = 0;
			break;
		}
	}

	/*not hintable*/
	if (!hintType) return NULL;
	/*we only support self-contained files for hinting*/
	gf_isom_get_data_reference(file, TrackNum, 1, &url, &urn);
	if (url || urn) return NULL;

	*e = GF_OUT_OF_MEM;
	GF_SAFEALLOC(tmp, GF_RTPHinter);
	if (!tmp) return NULL;

	/*override hinter type if requested and possible*/
	if (has_mpeg4_mapping && (flags & GP_RTP_PCK_FORCE_MPEG4)) {
		hintType = GF_RTP_PAYT_MPEG4;
		avc_nalu_size = 0;
	}
	/*use static payload ID if enabled*/
	else if (OfficialPayloadID && (flags & GP_RTP_PCK_USE_STATIC_ID) ) {
		PayloadID = OfficialPayloadID;
	}

	tmp->file = file;
	tmp->TrackNum = TrackNum;
	tmp->avc_nalu_size = avc_nalu_size;
	tmp->nb_chan = nb_ch;

	/*spatial scalability check*/
	tmp->has_ctts = gf_isom_has_time_offset(file, TrackNum);

	/*get sample info*/
	gf_media_get_sample_average_infos(file, TrackNum, &MinSize, &MaxSize, &avgTS, &maxDTSDelta, &const_dur, &bandwidth);

	/*systems carousel: we need at least IDX and RAP signaling*/
	if (flags & GP_RTP_PCK_SYSTEMS_CAROUSEL) {
		flags |= GP_RTP_PCK_SIGNAL_RAP;
	}

	/*update flags in MultiSL*/
	if (flags & GP_RTP_PCK_USE_MULTI) {
		if (MinSize != MaxSize) flags |= GP_RTP_PCK_SIGNAL_SIZE;
		if (!const_dur) flags |= GP_RTP_PCK_SIGNAL_TS;
	}
	if (tmp->has_ctts) flags |= GP_RTP_PCK_SIGNAL_TS;

	/*default SL for RTP */
	InitSL_RTP(&my_sl);

	my_sl.timestampResolution = gf_isom_get_media_timescale(file, TrackNum);
	/*override clockrate if set*/
	if (required_rate) {
		Double sc = required_rate;
		sc /= my_sl.timestampResolution;
		maxDTSDelta = (u32) (maxDTSDelta*sc);
		my_sl.timestampResolution = required_rate;
	}
	/*switch to RTP TS*/
	max_ptime = (u32) (max_ptime * my_sl.timestampResolution / 1000);

	my_sl.AUSeqNumLength = gf_get_bit_size(gf_isom_get_sample_count(file, TrackNum));
	if (my_sl.AUSeqNumLength>16) my_sl.AUSeqNumLength=16;

	my_sl.CUDuration = const_dur;

	if (gf_isom_has_sync_points(file, TrackNum)) {
		my_sl.useRandomAccessPointFlag = 1;
	} else {
		my_sl.useRandomAccessPointFlag = 0;
		my_sl.hasRandomAccessUnitsOnlyFlag = 1;
	}

	if (is_crypted) {
		Bool use_sel_enc;
		gf_isom_get_ismacryp_info(file, TrackNum, 1, NULL, NULL, NULL, NULL, NULL, &use_sel_enc, &IV_length, &KI_length);
		if (use_sel_enc) flags |= GP_RTP_PCK_SELECTIVE_ENCRYPTION;
	}

	// in case a different timescale was provided
	tmp->OrigTimeScale = gf_isom_get_media_timescale(file, TrackNum);
	tmp->rtp_p = gf_rtp_builder_new(hintType, &my_sl, flags, tmp,
	                                MP4T_OnNewPacket, MP4T_OnPacketDone,
	                                /*if copy, no data ref*/
	                                copy_media ? NULL : MP4T_OnDataRef,
	                                MP4T_OnData);

	//init the builder
	gf_rtp_builder_init(tmp->rtp_p, PayloadID, Path_MTU, max_ptime,
	                    streamType, codecid, PL_ID, MinSize, MaxSize, avgTS, maxDTSDelta, IV_length, KI_length, mpeg4mode);

	/*ISMA compliance is a pain...*/
	if (force_dts_delta) tmp->rtp_p->slMap.DTSDeltaLength = force_dts_delta;


	/*		Hint Track Setup	*/
	tmp->TrackID = gf_isom_get_track_id(file, TrackNum);
	tmp->HintID = tmp->TrackID + 65535;
	while (gf_isom_get_track_by_id(file, tmp->HintID)) tmp->HintID++;

	tmp->HintTrack = gf_isom_new_track(file, tmp->HintID, GF_ISOM_MEDIA_HINT, my_sl.timestampResolution);
	gf_isom_setup_hint_track(file, tmp->HintTrack, GF_ISOM_HINT_RTP);
	/*create a hint description*/
	gf_isom_new_hint_description(file, tmp->HintTrack, -1, -1, 0, &descIndex);
	gf_isom_rtp_set_timescale(file, tmp->HintTrack, descIndex, my_sl.timestampResolution);

	if (hintType==GF_RTP_PAYT_MPEG4) {
		tmp->rtp_p->slMap.CodecID = codecid;
		/*set this SL for extraction.*/
		*e = gf_isom_set_extraction_slc(file, TrackNum, 1, &my_sl);
		if (*e) {
			gf_hinter_track_del(tmp);
			return NULL;
		}
	}
	tmp->bandwidth = bandwidth;

	/*set interleaving*/
	gf_isom_set_track_interleaving_group(file, TrackNum, InterleaveGroupID);
	if (!copy_media) {
		/*if we don't copy data set hint track and media track in the same group*/
		gf_isom_set_track_interleaving_group(file, tmp->HintTrack, InterleaveGroupID);
	} else {
		gf_isom_set_track_interleaving_group(file, tmp->HintTrack, InterleaveGroupID + OFFSET_HINT_GROUP_ID);
	}
	/*use user-secified priority*/
	InterleaveGroupPriority*=2;
	gf_isom_set_track_priority_in_group(file, TrackNum, InterleaveGroupPriority+1);
	gf_isom_set_track_priority_in_group(file, tmp->HintTrack, InterleaveGroupPriority);

	*e = GF_OK;
	return tmp;
}