GF_Err gf_hinter_track_process(GF_RTPHinter *tkHint)
{
	GF_Err e;
	u32 i, descIndex, duration;
	u64 ts;
	u8 PadBits;
	GF_Fraction ft;
	GF_ISOSample *samp;

	tkHint->HintSample = tkHint->RTPTime = 0;

	tkHint->TotalSample = gf_isom_get_sample_count(tkHint->file, tkHint->TrackNum);
	ft.num = tkHint->rtp_p->sl_config.timestampResolution;
	ft.den = tkHint->OrigTimeScale;

	e = GF_OK;
	for (i=0; i<tkHint->TotalSample; i++) {
		samp = gf_isom_get_sample(tkHint->file, tkHint->TrackNum, i+1, &descIndex);
		if (!samp) return gf_isom_last_error(tkHint->file);

		//setup SL
		tkHint->CurrentSample = i + 1;

		/*keep same AU indicator if sync shadow - TODO FIXME: this assumes shadows are placed interleaved with
		the track content which is the case for GPAC scene carousel generation, but may not always be true*/
		if (samp->IsRAP==RAP_REDUNDANT) {
			tkHint->rtp_p->sl_header.AU_sequenceNumber -= 1;
			samp->IsRAP = RAP;
		}

		ts = ft.num * (samp->DTS+samp->CTS_Offset) / ft.den;
		tkHint->rtp_p->sl_header.compositionTimeStamp = ts;

		ts = ft.num * samp->DTS / ft.den;
		tkHint->rtp_p->sl_header.decodingTimeStamp = ts;
		tkHint->rtp_p->sl_header.randomAccessPointFlag = samp->IsRAP;

		tkHint->base_offset_in_sample = 0;
		/*crypted*/
		if (tkHint->rtp_p->slMap.IV_length) {
			GF_ISMASample *s = gf_isom_get_ismacryp_sample(tkHint->file, tkHint->TrackNum, samp, descIndex);
			/*one byte take for selective_enc flag*/
			if (s->flags & GF_ISOM_ISMA_USE_SEL_ENC) tkHint->base_offset_in_sample += 1;
			if (s->flags & GF_ISOM_ISMA_IS_ENCRYPTED) tkHint->base_offset_in_sample += s->IV_length + s->KI_length;
			gf_free(samp->data);
			samp->data = s->data;
			samp->dataLength = s->dataLength;
			gf_rtp_builder_set_cryp_info(tkHint->rtp_p, s->IV, (char*)s->key_indicator, (s->flags & GF_ISOM_ISMA_IS_ENCRYPTED) ? 1 : 0);
			s->data = NULL;
			s->dataLength = 0;
			gf_isom_ismacryp_delete_sample(s);
		}

		if (tkHint->rtp_p->sl_config.usePaddingFlag) {
			gf_isom_get_sample_padding_bits(tkHint->file, tkHint->TrackNum, i+1, &PadBits);
			tkHint->rtp_p->sl_header.paddingBits = PadBits;
		} else {
			tkHint->rtp_p->sl_header.paddingBits = 0;
		}

		duration = gf_isom_get_sample_duration(tkHint->file, tkHint->TrackNum, i+1);
//		ts = (u32) (ft * (s64) (duration));

		/*unpack nal units*/
		if (tkHint->avc_nalu_size) {
			u32 v, size;
			u32 remain = samp->dataLength;
			char *ptr = samp->data;

			tkHint->rtp_p->sl_header.accessUnitStartFlag = 1;
			tkHint->rtp_p->sl_header.accessUnitEndFlag = 0;
			while (remain) {
				size = 0;
				v = tkHint->avc_nalu_size;
				if (v>remain) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_RTP, ("[rtp hinter] Broken AVC nalu encapsulation: NALU size length is %d but only %d bytes left in sample %d\n", v, remain, tkHint->CurrentSample));
					break;
				}
				while (v) {
					size |= (u8) *ptr;
					ptr++;
					remain--;
					v-=1;
					if (v) size<<=8;
				}
				tkHint->base_offset_in_sample = samp->dataLength-remain;
				if (remain < size) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_RTP, ("[rtp hinter] Broken AVC nalu encapsulation: NALU size is %d but only %d bytes left in sample %d\n", size, remain, tkHint->CurrentSample));
					break;
				}
				remain -= size;
				tkHint->rtp_p->sl_header.accessUnitEndFlag = remain ? 0 : 1;
				e = gf_rtp_builder_process(tkHint->rtp_p, ptr, size, (u8) !remain, samp->dataLength, duration, (u8) (descIndex + GF_RTP_TX3G_SIDX_OFFSET) );
				ptr += size;
				tkHint->rtp_p->sl_header.accessUnitStartFlag = 0;
			}
		} else {
			e = gf_rtp_builder_process(tkHint->rtp_p, samp->data, samp->dataLength, 1, samp->dataLength, duration, (u8) (descIndex + GF_RTP_TX3G_SIDX_OFFSET) );
		}
		tkHint->rtp_p->sl_header.packetSequenceNumber += 1;

		//signal some progress
		gf_set_progress("Hinting", tkHint->CurrentSample, tkHint->TotalSample);

		tkHint->rtp_p->sl_header.AU_sequenceNumber += 1;
		gf_isom_sample_del(&samp);

		if (e) return e;
	}

	//flush
	gf_rtp_builder_process(tkHint->rtp_p, NULL, 0, 1, 0, 0, 0);

	gf_isom_end_hint_sample(tkHint->file, tkHint->HintTrack, (u8) tkHint->SampleIsRAP);
	return GF_OK;
}