	else if (!strcmp(canURN, "e2719d58-a985-b3c9-781a-b030af78d30e")) return "ClearKey1.0";
	else if (!strcmp(canURN, "94CE86FB-07FF-4F43-ADB8-93D2FA968CA2")) return "FairPlay";
	else if (!strcmp(canURN, "279fe473-512c-48fe-ade8-d176fee6b40f")) return "Arris Titanium";
	else if (!strcmp(canURN, "aa11967f-cc01-4a4a-8e99-c5d3dddfea2d")) return "UDRM";
	return "unknown";
}

static GF_List *dasher_get_content_protection_desc(GF_DasherCtx *ctx, GF_DashStream *ds, GF_MPD_AdaptationSet *for_set)
{
	char sCan[40];
	u32 prot_scheme=0;
	u32 i, count;
	const GF_PropertyValue *p;
	GF_List *res = NULL;
	GF_BitStream *bs_r;

	count = gf_list_count(ctx->current_period->streams);
	bs_r = gf_bs_new((const char *) &count, 1, GF_BITSTREAM_READ);

	for (i=0; i<count; i++) {
		GF_MPD_Descriptor *desc;
		GF_DashStream *a_ds = gf_list_get(ctx->current_period->streams, i);
		if (!a_ds->is_encrypted) continue;

		if (for_set) {
			if (a_ds->set != for_set) continue;
			//for now only insert for the stream holding the set
			if (!a_ds->owns_set) continue;
		} else if ((a_ds != ds) && (a_ds->muxed_base != ds) ) {
			continue;
		}

		p = gf_filter_pid_get_property(a_ds->ipid, GF_PROP_PID_PROTECTION_SCHEME_TYPE);
		if (p) prot_scheme = p->value.uint;

		if ((prot_scheme==GF_ISOM_CENC_SCHEME) || (prot_scheme==GF_ISOM_CBC_SCHEME) || (prot_scheme==GF_ISOM_CENS_SCHEME) || (prot_scheme==GF_ISOM_CBCS_SCHEME)
		) {
			const GF_PropertyValue *ki;
			u32 j, nb_pssh;
			GF_XMLAttribute *att;
			char szVal[GF_MAX_PATH];

			ctx->use_cenc = GF_TRUE;

			ki = gf_filter_pid_get_property(a_ds->ipid, GF_PROP_PID_CENC_KEY_INFO);
			if (!ki || !ki->value.data.ptr) {
				continue;
			}

			if (!res) res = gf_list_new();
			desc = gf_mpd_descriptor_new(NULL, "urn:mpeg:dash:mp4protection:2011", gf_4cc_to_str(prot_scheme));
			gf_list_add(res, desc);

			get_canon_urn(ki->value.data.ptr + 4, sCan);
			att = gf_xml_dom_create_attribute("cenc:default_KID", sCan);
			if (!desc->x_attributes) desc->x_attributes = gf_list_new();
			gf_list_add(desc->x_attributes, att);

			if (ctx->pssh <= GF_DASH_PSSH_MOOF) {
				continue;
			}
			//(data) binary blob containing (u32)N [(bin128)SystemID(u32)version(u32)KID_count[(bin128)keyID](u32)priv_size(char*priv_size)priv_data]
			p = gf_filter_pid_get_property(a_ds->ipid, GF_PROP_PID_CENC_PSSH);
			if (!p) continue;

			gf_bs_reassign_buffer(bs_r, p->value.data.ptr, p->value.data.size);
			nb_pssh = gf_bs_read_u32(bs_r);

			//add pssh
			for (j=0; j<nb_pssh; j++) {
				u32 pssh_idx;
				bin128 sysID;
				GF_XMLNode *node;
				u32 version, k_count;
				u8 *pssh_data=NULL;
				u32 pssh_len, size_64;
				GF_BitStream *bs_w = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);

				//rewrite PSSH box
				gf_bs_write_u32(bs_w, 0);
				gf_bs_write_u32(bs_w, GF_ISOM_BOX_TYPE_PSSH);

				gf_bs_read_data(bs_r, sysID, 16);
				version = gf_bs_read_u32(bs_r);

				k_count = version ? gf_bs_read_u32(bs_r) : 0;
				gf_bs_write_u8(bs_w, version);
				gf_bs_write_u24(bs_w, 0);
				gf_bs_write_data(bs_w, sysID, 16);
				if (version) {
					gf_bs_write_u32(bs_w, k_count);
					for (j=0; j<k_count; j++) {
						bin128 keyID;
						gf_bs_read_data(bs_r, keyID, 16);
						gf_bs_write_data(bs_w, keyID, 16);
					}
				}
				k_count = gf_bs_read_u32(bs_r);
				gf_bs_write_u32(bs_w, k_count);
				for (pssh_idx=0; pssh_idx<k_count; pssh_idx++) {
					gf_bs_write_u8(bs_w, gf_bs_read_u8(bs_r) );
				}
				pssh_len = (u32) gf_bs_get_position(bs_w);
				gf_bs_seek(bs_w, 0);
				gf_bs_write_u32(bs_w, pssh_len);
				gf_bs_seek(bs_w, pssh_len);
				gf_bs_get_content(bs_w, &pssh_data, &pssh_len);
				gf_bs_del(bs_w);

				get_canon_urn(sysID, sCan);
				desc = gf_mpd_descriptor_new(NULL, NULL, NULL);
				desc->x_children = gf_list_new();
				sprintf(szVal, "urn:uuid:%s", sCan);
				desc->scheme_id_uri = gf_strdup(szVal);
				desc->value = gf_strdup(get_drm_kms_name(sCan));
				gf_list_add(res, desc);

				GF_SAFEALLOC(node, GF_XMLNode);
				if (node) {
					GF_XMLNode *pnode;
					node->type = GF_XML_NODE_TYPE;
					node->name = gf_strdup("cenc:pssh");
					node->content = gf_list_new();
					gf_list_add(desc->x_children, node);

					GF_SAFEALLOC(pnode, GF_XMLNode);
					if (pnode) {
						pnode->type = GF_XML_TEXT_TYPE;
						gf_list_add(node->content, pnode);

						size_64 = 2*pssh_len;
						pnode->name = gf_malloc(size_64);
						if (pnode->name) {
							size_64 = gf_base64_encode((const char *)pssh_data, pssh_len, (char *)pnode->name, size_64);
							pnode->name[size_64] = 0;
						}
					}
				}
				gf_free(pssh_data);
			}
		} else {
			if (ctx->do_mpd) {
				GF_LOG(GF_LOG_WARNING, GF_LOG_DASH, ("[Dasher] Protection scheme %s has no official DASH mapping, using URI \"urn:gpac:dash:mp4protection:2018\"\n", gf_4cc_to_str(prot_scheme)));
			}
			if (!res) res = gf_list_new();