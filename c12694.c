static void nhmldump_send_frame(GF_NHMLDumpCtx *ctx, char *data, u32 data_size, GF_FilterPacket *pck)
{
	GF_FilterPacket *dst_pck;
	char nhml[1024];
	const GF_PropertyValue *p;
	u32 size;
	u8 *output;
	GF_FilterSAPType sap = gf_filter_pck_get_sap(pck);
	u64 dts = gf_filter_pck_get_dts(pck);
	u64 cts = gf_filter_pck_get_cts(pck);

	if (dts==GF_FILTER_NO_TS) dts = cts;
	if (cts==GF_FILTER_NO_TS) cts = dts;

	ctx->pck_num++;
	sprintf(nhml, "<NHNTSample number=\"%d\" DTS=\""LLU"\" dataLength=\"%d\" ", ctx->pck_num, dts, data_size);
	gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
	if (ctx->pckp || (cts != dts) ) {
		sprintf(nhml, "CTSOffset=\"%d\" ", (s32) ((s64)cts - (s64)dts));
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
	}
	if (sap==GF_FILTER_SAP_1) {
		sprintf(nhml, "isRAP=\"yes\" ");
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
	} else if (sap) {
		sprintf(nhml, "SAPType=\"%d\" ", sap);
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
	} else if (ctx->pckp) {
		sprintf(nhml, "isRAP=\"no\" ");
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		if ((sap==GF_FILTER_SAP_4) || (sap==GF_FILTER_SAP_4_PROL)) {
			s32 roll = gf_filter_pck_get_roll_info(pck);
			sprintf(nhml, "SAPType=\"4\" %s=\"%d\" ", (sap==GF_FILTER_SAP_4_PROL) ? "prol" : "roll", roll);
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		}
	}

	if (ctx->pckp) {
		u64 bo;
		u32 duration, idx;
		sprintf(nhml, "mediaOffset=\""LLU"\" ", ctx->mdia_pos);
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));

		bo = gf_filter_pck_get_byte_offset(pck);
		if (bo!=GF_FILTER_NO_BO) {
			sprintf(nhml, "sourceByteOffset=\""LLU"\" ", bo);
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		}
		duration = gf_filter_pck_get_duration(pck);
		if (duration) {
			sprintf(nhml, "duration=\"%d\" ", duration);
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		}
		idx = gf_filter_pck_get_carousel_version(pck);
		if (idx) {
			sprintf(nhml, "carouselVersion=\"%d\" ", idx);
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		}
		idx = 0;
		while (1) {
			u32 prop_4cc;
			const char *prop_name;
			p = gf_filter_pck_enum_properties(pck, &idx, &prop_4cc, &prop_name);
			if (!p) break;
			if (prop_4cc == GF_PROP_PCK_SUBS) continue;
			nhmldump_pck_property(ctx, prop_4cc, prop_name, p);
		}
	}

	if (ctx->chksum) {
		if (ctx->chksum==1) {
			u32 crc = gf_crc_32(data, data_size);
			sprintf(nhml, "crc=\"%08X\" ", crc);
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		} else {
			u32 j;
			u8 hash[GF_SHA1_DIGEST_SIZE];
			gf_sha1_csum(data, data_size, hash);
			sprintf(nhml, "sha1=\"");
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
			for (j=0; j<20; j++) {
				sprintf(nhml, "%02X", hash[j]);
				gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
			}
			sprintf(nhml, "\" ");
			gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		}
	}

	sprintf(nhml, ">\n");
	gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));

	p = gf_filter_pck_get_property(pck, GF_PROP_PCK_SUBS);
	if (p) {
		u32 offset_in_sample = 0;
		Bool first_subs = GF_TRUE;
		if (!ctx->bs_r) ctx->bs_r = gf_bs_new(p->value.data.ptr, p->value.data.size, GF_BITSTREAM_READ);
		else gf_bs_reassign_buffer(ctx->bs_r, p->value.data.ptr, p->value.data.size);

		//(data) binary blob containing N [(u32)flags(u32)size(u32)reserved(u8)priority(u8) discardable]
		while (gf_bs_available(ctx->bs_r)) {
			u32 s_flags = gf_bs_read_u32(ctx->bs_r);
			u32 s_size = gf_bs_read_u32(ctx->bs_r);
			u32 s_res = gf_bs_read_u32(ctx->bs_r);
			u8 s_prio = gf_bs_read_u8(ctx->bs_r);
			u8 s_discard = gf_bs_read_u8(ctx->bs_r);


			if (offset_in_sample + s_size > data_size) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_MEDIA, ("Wrong subsample info: sample size %d vs subsample offset+size %dn", data_size, offset_in_sample + s_size));
				break;
			}

			if (ctx->is_stpp && ctx->nhmlonly) {
				if (first_subs) {
					sprintf(nhml, "<NHNTSubSample>\n");
					gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));

					gf_bs_write_data(ctx->bs_w, data, s_size);

					sprintf(nhml, "</NHNTSubSample>\n");
					gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
				} else {
					u32 d_size;
					if (ctx->b64_buffer_size<2*s_size) {
						ctx->b64_buffer_size = 2 * s_size;
						ctx->b64_buffer = gf_realloc(ctx->b64_buffer, ctx->b64_buffer_size);
					}
					d_size = gf_base64_encode(data + offset_in_sample, s_size, ctx->b64_buffer, ctx->b64_buffer_size);
					ctx->b64_buffer[d_size] = 0;
					sprintf(nhml, "<NHNTSubSample data=\"data:application/octet-string;base64,");
					gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
					gf_bs_write_data(ctx->bs_w, ctx->b64_buffer, d_size);
					sprintf(nhml, "\">\n");
					gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
				}
			} else {
				sprintf(nhml, "<NHNTSubSample size=\"%d\" flags=\"%d\" reserved=\"%d\" priority=\"%d\" discard=\"%d\" />\n", s_size, s_flags, s_res, s_prio, s_discard);
				gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
			}
			first_subs = GF_FALSE;
		}
	} else if (ctx->is_stpp && ctx->nhmlonly) {
		sprintf(nhml, "<NHNTSubSample><![CDATA[\n");
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
		gf_bs_write_data(ctx->bs_w, data, data_size);
		sprintf(nhml, "]]></NHNTSubSample>\n");
		gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));
	}
	sprintf(nhml, "</NHNTSample>\n");
	gf_bs_write_data(ctx->bs_w, nhml, (u32) strlen(nhml));

	gf_bs_get_content_no_truncate(ctx->bs_w, &ctx->nhml_buffer, &size, &ctx->nhml_buffer_size);

	if (ctx->filep) {
		gf_fwrite(ctx->nhml_buffer, size, ctx->filep);
		return;
	}

	dst_pck = gf_filter_pck_new_alloc(ctx->opid_nhml, size, &output);
	if (dst_pck) {
		memcpy(output, ctx->nhml_buffer, size);
		gf_filter_pck_set_framing(dst_pck, GF_FALSE, GF_FALSE);
		gf_filter_pck_send(dst_pck);
	}

	ctx->mdia_pos += data_size;

	if (ctx->opid_mdia) {
		//send the complete data packet
		dst_pck = gf_filter_pck_new_ref(ctx->opid_mdia, 0, data_size, pck);
		if (!dst_pck) return;

		gf_filter_pck_merge_properties(pck, dst_pck);
		//keep byte offset ?
//		gf_filter_pck_set_byte_offset(dst_pck, GF_FILTER_NO_BO);

		gf_filter_pck_set_framing(dst_pck, ctx->first, GF_FALSE);
		gf_filter_pck_send(dst_pck);
	}
}