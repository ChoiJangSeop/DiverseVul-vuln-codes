GF_Err gf_hinter_track_finalize(GF_RTPHinter *tkHint, Bool AddSystemInfo)
{
	u32 Width, Height;
	GF_ESD *esd;
	char sdpLine[20000];
	char mediaName[30], payloadName[30];
    u32 mtype;

	Width = Height = 0;
	gf_isom_sdp_clean_track(tkHint->file, tkHint->TrackNum);
    mtype = gf_isom_get_media_type(tkHint->file, tkHint->TrackNum);
    if (gf_isom_is_video_handler_type(mtype))
		gf_isom_get_visual_info(tkHint->file, tkHint->TrackNum, 1, &Width, &Height);

	gf_rtp_builder_get_payload_name(tkHint->rtp_p, payloadName, mediaName);

	/*TODO- extract out of rtp_p for future live tools*/
	sprintf(sdpLine, "m=%s 0 RTP/%s %d", mediaName, tkHint->rtp_p->slMap.IV_length ? "SAVP" : "AVP", tkHint->rtp_p->PayloadType);
	gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	if (tkHint->bandwidth) {
		sprintf(sdpLine, "b=AS:%d", tkHint->bandwidth);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	if (tkHint->nb_chan) {
		sprintf(sdpLine, "a=rtpmap:%d %s/%d/%d", tkHint->rtp_p->PayloadType, payloadName, tkHint->rtp_p->sl_config.timestampResolution, tkHint->nb_chan);
	} else {
		sprintf(sdpLine, "a=rtpmap:%d %s/%d", tkHint->rtp_p->PayloadType, payloadName, tkHint->rtp_p->sl_config.timestampResolution);
	}
	gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	/*control for MPEG-4*/
	if (AddSystemInfo) {
		sprintf(sdpLine, "a=mpeg4-esid:%d", gf_isom_get_track_id(tkHint->file, tkHint->TrackNum));
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*control for QTSS/DSS*/
	sprintf(sdpLine, "a=control:trackID=%d", gf_isom_get_track_id(tkHint->file, tkHint->HintTrack));
	gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);

	/*H263 extensions*/
	if (tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_H263) {
		sprintf(sdpLine, "a=cliprect:0,0,%d,%d", Height, Width);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*AMR*/
	else if ((tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_AMR) || (tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_AMR_WB)) {
		sprintf(sdpLine, "a=fmtp:%d octet-align=1", tkHint->rtp_p->PayloadType);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*Text*/
	else if (tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_3GPP_TEXT) {
		u32 w, h, i, m_w, m_h;
		s32 tx, ty;
		s16 l;

		gf_isom_get_track_layout_info(tkHint->file, tkHint->TrackNum, &w, &h, &tx, &ty, &l);
		m_w = w;
		m_h = h;
		for (i=0; i<gf_isom_get_track_count(tkHint->file); i++) {
			switch (gf_isom_get_media_type(tkHint->file, i+1)) {
			case GF_ISOM_MEDIA_SCENE:
			case GF_ISOM_MEDIA_VISUAL:
			case GF_ISOM_MEDIA_AUXV:
			case GF_ISOM_MEDIA_PICT:
				gf_isom_get_track_layout_info(tkHint->file, i+1, &w, &h, &tx, &ty, &l);
				if (w>m_w) m_w = w;
				if (h>m_h) m_h = h;
				break;
			default:
				break;
			}
		}

		gf_media_format_ttxt_sdp(tkHint->rtp_p, payloadName, sdpLine, w, h, tx, ty, l, m_w, m_h, NULL);

		strcat(sdpLine, "; tx3g=");
		for (i=0; i<gf_isom_get_sample_description_count(tkHint->file, tkHint->TrackNum); i++) {
			u8 *tx3g;
			char buffer[2000];
			u32 tx3g_len, len;
			gf_isom_text_get_encoded_tx3g(tkHint->file, tkHint->TrackNum, i+1, GF_RTP_TX3G_SIDX_OFFSET, &tx3g, &tx3g_len);
			len = gf_base64_encode(tx3g, tx3g_len, buffer, 2000);
			gf_free(tx3g);
			buffer[len] = 0;
			if (i) strcat(sdpLine, ", ");
			strcat(sdpLine, buffer);
		}
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*EVRC/SMV in non header-free mode*/
	else if ((tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_EVRC_SMV) && (tkHint->rtp_p->auh_size>1)) {
		sprintf(sdpLine, "a=fmtp:%d maxptime=%d", tkHint->rtp_p->PayloadType, tkHint->rtp_p->auh_size*20);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*H264/AVC*/
	else if ((tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_H264_AVC) || (tkHint->rtp_p->rtp_payt == GF_RTP_PAYT_H264_SVC))  {
		GF_AVCConfig *avcc = gf_isom_avc_config_get(tkHint->file, tkHint->TrackNum, 1);
		GF_AVCConfig *svcc = gf_isom_svc_config_get(tkHint->file, tkHint->TrackNum, 1);
		/*TODO - check syntax for SVC (might be some extra signaling)*/

		if (avcc) {
			sprintf(sdpLine, "a=fmtp:%d profile-level-id=%02X%02X%02X; packetization-mode=1", tkHint->rtp_p->PayloadType, avcc->AVCProfileIndication, avcc->profile_compatibility, avcc->AVCLevelIndication);
		} else {
			sprintf(sdpLine, "a=fmtp:%d profile-level-id=%02X%02X%02X; packetization-mode=1", tkHint->rtp_p->PayloadType, svcc->AVCProfileIndication, svcc->profile_compatibility, svcc->AVCLevelIndication);
		}

		write_avc_config(sdpLine, avcc, svcc);

		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
		gf_odf_avc_cfg_del(avcc);
		gf_odf_avc_cfg_del(svcc);
	}
	/*MPEG-4 decoder config*/
	else if (tkHint->rtp_p->rtp_payt==GF_RTP_PAYT_MPEG4) {
		esd = gf_isom_get_esd(tkHint->file, tkHint->TrackNum, 1);

		if (esd && esd->decoderConfig && esd->decoderConfig->decoderSpecificInfo && esd->decoderConfig->decoderSpecificInfo->data) {
			gf_rtp_builder_format_sdp(tkHint->rtp_p, payloadName, sdpLine, esd->decoderConfig->decoderSpecificInfo->data, esd->decoderConfig->decoderSpecificInfo->dataLength);
		} else {
			gf_rtp_builder_format_sdp(tkHint->rtp_p, payloadName, sdpLine, NULL, 0);
		}
		if (esd) gf_odf_desc_del((GF_Descriptor *)esd);

		if (tkHint->rtp_p->slMap.IV_length) {
			const char *kms;
			gf_isom_get_ismacryp_info(tkHint->file, tkHint->TrackNum, 1, NULL, NULL, NULL, NULL, &kms, NULL, NULL, NULL);
			if (!strnicmp(kms, "(key)", 5) || !strnicmp(kms, "(ipmp)", 6) || !strnicmp(kms, "(uri)", 5)) {
				strcat(sdpLine, "; ISMACrypKey=");
			} else {
				strcat(sdpLine, "; ISMACrypKey=(uri)");
			}
			strcat(sdpLine, kms);
		}

		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	/*MPEG-4 Audio LATM*/
	else if (tkHint->rtp_p->rtp_payt==GF_RTP_PAYT_LATM) {
		GF_BitStream *bs;
		u8 *config_bytes;
		u32 config_size;

		/* form config string */
		bs = gf_bs_new(NULL, 32, GF_BITSTREAM_WRITE);
		gf_bs_write_int(bs, 0, 1); /* AudioMuxVersion */
		gf_bs_write_int(bs, 1, 1); /* all streams same time */
		gf_bs_write_int(bs, 0, 6); /* numSubFrames */
		gf_bs_write_int(bs, 0, 4); /* numPrograms */
		gf_bs_write_int(bs, 0, 3); /* numLayer */

		/* audio-specific config */
		esd = gf_isom_get_esd(tkHint->file, tkHint->TrackNum, 1);
		if (esd && esd->decoderConfig && esd->decoderConfig->decoderSpecificInfo) {
			/*PacketVideo patch: don't signal SBR and PS stuff, not allowed in LATM with audioMuxVersion=0*/
			gf_bs_write_data(bs, esd->decoderConfig->decoderSpecificInfo->data, MIN(esd->decoderConfig->decoderSpecificInfo->dataLength, 2) );
		}
		if (esd) gf_odf_desc_del((GF_Descriptor *)esd);

		/* other data */
		gf_bs_write_int(bs, 0, 3); /* frameLengthType */
		gf_bs_write_int(bs, 0xff, 8); /* latmBufferFullness */
		gf_bs_write_int(bs, 0, 1); /* otherDataPresent */
		gf_bs_write_int(bs, 0, 1); /* crcCheckPresent */
		gf_bs_get_content(bs, &config_bytes, &config_size);
		gf_bs_del(bs);

		gf_rtp_builder_format_sdp(tkHint->rtp_p, payloadName, sdpLine, config_bytes, config_size);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
		gf_free(config_bytes);
	}
#if GPAC_ENABLE_3GPP_DIMS_RTP
	/*3GPP DIMS*/
	else if (tkHint->rtp_p->rtp_payt==GF_RTP_PAYT_3GPP_DIMS) {
		GF_DIMSDescription dims;
		gf_isom_get_visual_info(tkHint->file, tkHint->TrackNum, 1, &Width, &Height);

		gf_isom_get_dims_description(tkHint->file, tkHint->TrackNum, 1, &dims);
		sprintf(sdpLine, "a=fmtp:%d Version-profile=%d", tkHint->rtp_p->PayloadType, dims.profile);
		if (! dims.fullRequestHost) {
			char fmt[200];
			strcat(sdpLine, ";useFullRequestHost=0");
			sprintf(fmt, ";pathComponents=%d", dims.pathComponents);
			strcat(sdpLine, fmt);
		}
		if (!dims.streamType) strcat(sdpLine, ";stream-type=secondary");
		if (dims.containsRedundant == 1) strcat(sdpLine, ";contains-redundant=main");
		else if (dims.containsRedundant == 2) strcat(sdpLine, ";contains-redundant=redundant");

		if (dims.textEncoding && strlen(dims.textEncoding)) {
			strcat(sdpLine, ";text-encoding=");
			strcat(sdpLine, dims.textEncoding);
		}
		if (dims.contentEncoding && strlen(dims.contentEncoding)) {
			strcat(sdpLine, ";content-coding=");
			strcat(sdpLine, dims.contentEncoding);
		}
		if (dims.contentEncoding && dims.content_script_types && strlen(dims.content_script_types) ) {
			strcat(sdpLine, ";content-script-types=");
			strcat(sdpLine, dims.contentEncoding);
		}
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
#endif
	/*extensions for some mobile phones*/
	if (Width && Height) {
		sprintf(sdpLine, "a=framesize:%d %d-%d", tkHint->rtp_p->PayloadType, Width, Height);
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}

	esd = gf_isom_get_esd(tkHint->file, tkHint->TrackNum, 1);
	if (esd && esd->decoderConfig && (esd->decoderConfig->rvc_config || esd->decoderConfig->predefined_rvc_config)) {
		if (esd->decoderConfig->predefined_rvc_config) {
			sprintf(sdpLine, "a=rvc-config-predef:%d", esd->decoderConfig->predefined_rvc_config);
		} else {
			/*temporary ...*/
			if ((esd->decoderConfig->objectTypeIndication==GF_CODECID_AVC) || (esd->decoderConfig->objectTypeIndication==GF_CODECID_SVC)) {
				sprintf(sdpLine, "a=rvc-config:%s", "http://download.tsi.telecom-paristech.fr/gpac/RVC/rvc_config_avc.xml");
			} else {
				sprintf(sdpLine, "a=rvc-config:%s", "http://download.tsi.telecom-paristech.fr/gpac/RVC/rvc_config_sp.xml");
			}
		}
		gf_isom_sdp_add_track_line(tkHint->file, tkHint->HintTrack, sdpLine);
	}
	if (esd) gf_odf_desc_del((GF_Descriptor *)esd);

	gf_isom_set_track_enabled(tkHint->file, tkHint->HintTrack, GF_TRUE);
	return GF_OK;
}