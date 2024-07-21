static void cryptinfo_node_start(void *sax_cbck, const char *node_name, const char *name_space, const GF_XMLAttribute *attributes, u32 nb_attributes)
{
	GF_XMLAttribute *att;
	GF_TrackCryptInfo *tkc;
	u32 i;
	GF_CryptInfo *info = (GF_CryptInfo *)sax_cbck;

	if (!strcmp(node_name, "OMATextHeader")) {
		info->in_text_header = 1;
		return;
	}
	if (!strcmp(node_name, "GPACDRM")) {
		for (i=0; i<nb_attributes; i++) {
			att = (GF_XMLAttribute *) &attributes[i];
			if (!stricmp(att->name, "type")) {
				info->def_crypt_type = cryptinfo_get_crypt_type(att->value);
			}
		}
		return;
	}


	if (!strcmp(node_name, "CrypTrack")) {
		Bool has_key = GF_FALSE;
		Bool has_common_key = GF_TRUE;
		GF_SAFEALLOC(tkc, GF_TrackCryptInfo);
		if (!tkc) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] Cannnot allocate crypt track, skipping\n"));
			info->last_parse_error = GF_OUT_OF_MEM;
			return;
		}
		//by default track is encrypted
		tkc->IsEncrypted = 1;
		tkc->sai_saved_box_type = GF_ISOM_BOX_TYPE_SENC;
		tkc->scheme_type = info->def_crypt_type;

		//allocate a key to store the default values in single-key mode
		tkc->keys = gf_malloc(sizeof(GF_CryptKeyInfo));
		if (!tkc->keys) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] Cannnot allocate key IDs\n"));
			gf_free(tkc);
			info->last_parse_error = GF_OUT_OF_MEM;
			return;
		}
		memset(tkc->keys, 0, sizeof(GF_CryptKeyInfo));
		gf_list_add(info->tcis, tkc);

		for (i=0; i<nb_attributes; i++) {
			att = (GF_XMLAttribute *) &attributes[i];
			if (!stricmp(att->name, "trackID") || !stricmp(att->name, "ID")) {
				if (!strcmp(att->value, "*")) info->has_common_key = 1;
				else {
					tkc->trackID = atoi(att->value);
					has_common_key = GF_FALSE;
				}
			}
			else if (!stricmp(att->name, "type")) {
				tkc->scheme_type = cryptinfo_get_crypt_type(att->value);
			}
			else if (!stricmp(att->name, "forceType")) {
				tkc->force_type = GF_TRUE;
			}
			else if (!stricmp(att->name, "key")) {
				GF_Err e;
				has_key = GF_TRUE;
				e = gf_bin128_parse(att->value, tkc->keys[0].key );
                if (e != GF_OK) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Cannnot parse key value in CrypTrack\n"));
					info->last_parse_error = GF_BAD_PARAM;
                    return;
                }
			}
			else if (!stricmp(att->name, "salt")) {
				u32 len, j;
				char *sKey = att->value;
				if (!strnicmp(sKey, "0x", 2)) sKey += 2;
				len = (u32) strlen(sKey);
				for (j=0; j<len; j+=2) {
					char szV[5];
					u32 v;
					sprintf(szV, "%c%c", sKey[j], sKey[j+1]);
					sscanf(szV, "%x", &v);
					tkc->keys[0].IV[j/2] = v;
					if (j>=30) break;
				}
			}
			else if (!stricmp(att->name, "kms_URI") || !stricmp(att->name, "rightsIssuerURL")) {
				if (tkc->KMS_URI) gf_free(tkc->KMS_URI);
				tkc->KMS_URI = gf_strdup(att->value);
			}
			else if (!stricmp(att->name, "scheme_URI") || !stricmp(att->name, "contentID")) {
				if (tkc->Scheme_URI) gf_free(tkc->Scheme_URI);
				tkc->Scheme_URI = gf_strdup(att->value);
			}
			else if (!stricmp(att->name, "selectiveType")) {
				if (!stricmp(att->value, "Rap")) tkc->sel_enc_type = GF_CRYPT_SELENC_RAP;
				else if (!stricmp(att->value, "Non-Rap")) tkc->sel_enc_type = GF_CRYPT_SELENC_NON_RAP;
				else if (!stricmp(att->value, "Rand")) tkc->sel_enc_type = GF_CRYPT_SELENC_RAND;
				else if (!strnicmp(att->value, "Rand", 4)) {
					tkc->sel_enc_type = GF_CRYPT_SELENC_RAND_RANGE;
					tkc->sel_enc_range = atoi(&att->value[4]);
				}
				else if (sscanf(att->value, "%u", &tkc->sel_enc_range)==1) {
					if (tkc->sel_enc_range==1) tkc->sel_enc_range = 0;
					else tkc->sel_enc_type = GF_CRYPT_SELENC_RANGE;
				}
				else if (!strnicmp(att->value, "Preview", 7)) {
					tkc->sel_enc_type = GF_CRYPT_SELENC_PREVIEW;
				}
				else if (!strnicmp(att->value, "Clear", 5)) {
					tkc->sel_enc_type = GF_CRYPT_SELENC_CLEAR;
				}
				else if (!strnicmp(att->value, "ForceClear", 10)) {
					char *sep = strchr(att->value, '=');
					if (sep) tkc->sel_enc_range = atoi(sep+1);
					tkc->sel_enc_type = GF_CRYPT_SELENC_CLEAR_FORCED;
				}
				else if (!stricmp(att->value, "None")) {
				} else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized selective mode %s, ignoring\n", att->value));
				}
			}
			else if (!stricmp(att->name, "clearStsd")) {
				if (!strcmp(att->value, "none")) tkc->force_clear_stsd_idx = 0;
				else if (!strcmp(att->value, "before")) tkc->force_clear_stsd_idx = 1;
				else if (!strcmp(att->value, "after")) tkc->force_clear_stsd_idx = 2;
				else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized clear stsd type %s, defaulting to no stsd for clear samples\n", att->value));
				}
			}
			else if (!stricmp(att->name, "Preview")) {
				tkc->sel_enc_type = GF_CRYPT_SELENC_PREVIEW;
				sscanf(att->value, "%u", &tkc->sel_enc_range);
			}
			else if (!stricmp(att->name, "ipmpType")) {
				if (!stricmp(att->value, "None")) tkc->ipmp_type = 0;
				else if (!stricmp(att->value, "IPMP")) tkc->sel_enc_type = 1;
				else if (!stricmp(att->value, "IPMPX")) tkc->sel_enc_type = 2;
			}
			else if (!stricmp(att->name, "ipmpDescriptorID")) tkc->ipmp_desc_id = atoi(att->value);
			else if (!stricmp(att->name, "encryptionMethod")) {
				if (!strcmp(att->value, "AES_128_CBC")) tkc->encryption = 1;
				else if (!strcmp(att->value, "None")) tkc->encryption = 0;
				else if (!strcmp(att->value, "AES_128_CTR") || !strcmp(att->value, "default")) tkc->encryption = 2;
				else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized encryption algo %s, ignoring\n", att->value));
				}
			}
			else if (!stricmp(att->name, "transactionID")) {
				if (strlen(att->value)<=16) strcpy(tkc->TransactionID, att->value);
			}
			else if (!stricmp(att->name, "textualHeaders")) {
			}
			else if (!stricmp(att->name, "IsEncrypted")) {
				if (!stricmp(att->value, "1"))
					tkc->IsEncrypted = 1;
				else
					tkc->IsEncrypted = 0;
			}
			else if (!stricmp(att->name, "IV_size")) {
				tkc->keys[0].IV_size = atoi(att->value);
			}
			else if (!stricmp(att->name, "first_IV")) {
				char *sKey = att->value;
				if (!strnicmp(sKey, "0x", 2)) sKey += 2;
				if ((strlen(sKey) == 16) || (strlen(sKey) == 32)) {
					u32 j;
					for (j=0; j<strlen(sKey); j+=2) {
						u32 v;
						char szV[5];
						sprintf(szV, "%c%c", sKey[j], sKey[j+1]);
						sscanf(szV, "%x", &v);
						tkc->keys[0].IV[j/2] = v;
					}
					if (!tkc->keys[0].IV_size) tkc->keys[0].IV_size = (u8) strlen(sKey) / 2;
				}
			}
			else if (!stricmp(att->name, "saiSavedBox")) {
				if (!stricmp(att->value, "uuid_psec")) tkc->sai_saved_box_type = GF_ISOM_BOX_UUID_PSEC;
				else if (!stricmp(att->value, "senc")) tkc->sai_saved_box_type = GF_ISOM_BOX_TYPE_SENC;
				else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized SAI location %s, ignoring\n", att->value));
				}
			}
			else if (!stricmp(att->name, "keyRoll")) {
				if (!strncmp(att->value, "idx=", 4))
					tkc->defaultKeyIdx = atoi(att->value+4);
				else if (!strncmp(att->value, "roll", 4) || !strncmp(att->value, "samp", 4)) {
					tkc->roll_type = GF_KEYROLL_SAMPLES;
					if (att->value[4]=='=') tkc->keyRoll = atoi(att->value+5);
					if (!tkc->keyRoll) tkc->keyRoll = 1;
				}
				else if (!strncmp(att->value, "seg", 3)) {
					tkc->roll_type = GF_KEYROLL_SEGMENTS;
					if (att->value[3]=='=') tkc->keyRoll = atoi(att->value+4);
					if (!tkc->keyRoll) tkc->keyRoll = 1;
				} else if (!strncmp(att->value, "period", 6)) {
					tkc->roll_type = GF_KEYROLL_PERIODS;
					if (att->value[6]=='=') tkc->keyRoll = atoi(att->value+7);
					if (!tkc->keyRoll) tkc->keyRoll = 1;
				} else if (!strcmp(att->value, "rap")) {
					tkc->roll_type = GF_KEYROLL_SAPS;
					if (att->value[3]=='=') tkc->keyRoll = atoi(att->value+4);
					if (!tkc->keyRoll) tkc->keyRoll = 1;
				} else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized roll parameter %s, ignoring\n", att->value));
				}
			}
			else if (!stricmp(att->name, "random")) {
				if (!strcmp(att->value, "true") || !strcmp(att->value, "1") || !strcmp(att->value, "yes")) {
					tkc->rand_keys=GF_TRUE;
				}
				else if (!strcmp(att->value, "false") || !strcmp(att->value, "0") || !strcmp(att->value, "no")) {
					tkc->rand_keys=GF_FALSE;
				}
				else {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Unrecognized random parameter %s, ignoring\n", att->value));
				}
			}
			else if (!stricmp(att->name, "metadata")) {
				u32 l = 2 * (u32) strlen(att->value);
				tkc->metadata = gf_malloc(sizeof(char) * l);
				l = gf_base64_encode(att->value, (u32) strlen(att->value), tkc->metadata, l);
				tkc->metadata[l] = 0;
			}
			else if (!stricmp(att->name, "crypt_byte_block")) {
				tkc->crypt_byte_block = atoi(att->value);
			}
			else if (!stricmp(att->name, "skip_byte_block")) {
				tkc->skip_byte_block = atoi(att->value);
			}
			else if (!stricmp(att->name, "clear_bytes")) {
				tkc->clear_bytes = atoi(att->value);
			}
			else if (!stricmp(att->name, "constant_IV_size")) {
				tkc->keys[0].constant_IV_size = atoi(att->value);
				if ((tkc->keys[0].constant_IV_size != 8) && (tkc->keys[0].constant_IV_size != 16)) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Constant IV size %d is not 8 or 16\n", att->value));
				}
			}
			else if (!stricmp(att->name, "constant_IV")) {
				char *sKey = att->value;
				if (!strnicmp(sKey, "0x", 2)) sKey += 2;
				if ((strlen(sKey) == 16) || (strlen(sKey) == 32)) {
					u32 j;
					for (j=0; j<strlen(sKey); j+=2) {
						u32 v;
						char szV[5];
						sprintf(szV, "%c%c", sKey[j], sKey[j+1]);
						sscanf(szV, "%x", &v);
						tkc->keys[0].IV[j/2] = v;
					}
					if (!tkc->keys[0].constant_IV_size) tkc->keys[0].constant_IV_size = (u8) strlen(sKey) / 2;
				}
			}
			else if (!stricmp(att->name, "encryptSliceHeader")) {
				tkc->allow_encrypted_slice_header = !strcmp(att->value, "yes") ? GF_TRUE : GF_FALSE;
			}
			else if (!stricmp(att->name, "blockAlign")) {
				if (!strcmp(att->value, "disable")) tkc->block_align = 1;
				else if (!strcmp(att->value, "always")) tkc->block_align = 2;
				else tkc->block_align = 0;
			}
			else if (!stricmp(att->name, "subsamples")) {
				char *val = att->value;
				while (val) {
					char *sep = strchr(val, ';');
					if (sep) sep[0] = 0;
					if (!strncmp(val, "subs=", 5)) {
						if (tkc->subs_crypt) gf_free(tkc->subs_crypt);
						tkc->subs_crypt = gf_strdup(val+4);
					}
					else if (!strncmp(val, "rand", 4)) {
						tkc->subs_rand = 2;
						if (val[4]=='=')
							tkc->subs_rand = atoi(val+5);
					}
					else {
						GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] unrecognized attribute value %s for `subsamples`, ignoring\n", val));
					}
					if (!sep) break;
					sep[0] = ';';
					val = sep+1;
				}
			}
			else if (!stricmp(att->name, "multiKey")) {
				if (!strcmp(att->value, "all") || !strcmp(att->value, "on")) tkc->multi_key = GF_TRUE;
				else if (!strcmp(att->value, "no")) tkc->multi_key = GF_FALSE;
				else {
					char *val = att->value;
					tkc->multi_key = GF_TRUE;
					while (val) {
						char *sep = strchr(val, ';');
						if (sep) sep[0] = 0;
						if (!strncmp(val, "roll=", 5)) {
							tkc->mkey_roll_plus_one = 1 + atoi(val+5);
						}
						else if (!strncmp(val, "subs=", 5)) {
							if (tkc->mkey_subs) gf_free(tkc->mkey_subs);
							tkc->mkey_subs = gf_strdup(val+5);
						}
						else {
							GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] unrecognized attribute value %s for `multiKey`, ignoring\n", val));
							tkc->multi_key = GF_FALSE;
							if (sep) sep[0] = ';';
							break;
						}
						if (!sep) break;
						sep[0] = ';';
						val = sep+1;
					}
				}
			}
		}
		if (tkc->scheme_type==GF_CRYPT_TYPE_PIFF) {
			tkc->sai_saved_box_type = GF_ISOM_BOX_UUID_PSEC;
		}
		if (has_common_key) info->has_common_key = 1;

		if ((tkc->keys[0].IV_size != 0) && (tkc->keys[0].IV_size != 8) && (tkc->keys[0].IV_size != 16)) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] wrong IV size %d for AES-128, using 16\n", (u32) tkc->keys[0].IV_size));
			tkc->keys[0].IV_size = 16;
		}

		if ((tkc->scheme_type == GF_CRYPT_TYPE_CENC) || (tkc->scheme_type == GF_CRYPT_TYPE_CBC1)) {
			if (tkc->crypt_byte_block || tkc->skip_byte_block) {
				GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] Using scheme type %s, crypt_byte_block and skip_byte_block shall be 0\n", gf_4cc_to_str(tkc->scheme_type) ));
				tkc->crypt_byte_block = tkc->skip_byte_block = 0;
			}
		}

		if ((tkc->scheme_type == GF_CRYPT_TYPE_CENC) || (tkc->scheme_type == GF_CRYPT_TYPE_CBC1) || (tkc->scheme_type == GF_CRYPT_TYPE_CENS)) {
			if (tkc->keys[0].constant_IV_size) {
				if (!tkc->keys[0].IV_size) {
					tkc->keys[0].IV_size = tkc->keys[0].constant_IV_size;
					GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] Using scheme type %s, constant IV shall not be used, using constant IV as first IV\n", gf_4cc_to_str(tkc->scheme_type)));
					tkc->keys[0].constant_IV_size = 0;
				} else {
					tkc->keys[0].constant_IV_size = 0;
					GF_LOG(GF_LOG_WARNING, GF_LOG_PARSER, ("[CENC] Using scheme type %s, constant IV shall not be used, ignoring\n", gf_4cc_to_str(tkc->scheme_type)));
				}
			}
		}
		if (tkc->scheme_type == GF_ISOM_OMADRM_SCHEME) {
			/*default to AES 128 in OMA*/
			tkc->encryption = 2;
		}

		if (has_key) tkc->nb_keys = 1;
	}

	if (!strcmp(node_name, "key")) {
		u32 IV_size, const_IV_size;
		Bool kas_civ = GF_FALSE;
		tkc = (GF_TrackCryptInfo *)gf_list_last(info->tcis);
		if (!tkc) return;
		//only realloc for 2nd and more
		if (tkc->nb_keys) {
			tkc->keys = (GF_CryptKeyInfo *)gf_realloc(tkc->keys, sizeof(GF_CryptKeyInfo)*(tkc->nb_keys+1));
			memset(&tkc->keys[tkc->nb_keys], 0, sizeof(GF_CryptKeyInfo));
		}
		IV_size = tkc->keys[0].IV_size;
		const_IV_size = tkc->keys[0].constant_IV_size;

		for (i=0; i<nb_attributes; i++) {
			att = (GF_XMLAttribute *) &attributes[i];

			if (!stricmp(att->name, "KID")) {
				GF_Err e = gf_bin128_parse(att->value, tkc->keys[tkc->nb_keys].KID);
                if (e != GF_OK) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Cannnot parse KID\n"));
                    return;
                }
			}
			else if (!stricmp(att->name, "value")) {
				GF_Err e = gf_bin128_parse(att->value, tkc->keys[tkc->nb_keys].key);
                if (e != GF_OK) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Cannnot parse key value\n"));
                    return;
                }
			}
			else if (!stricmp(att->name, "hlsInfo")) {
				if (!strstr(att->value, "URI=\"")) {
                    GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("[CENC] Missing URI in HLS info %s\n", att->value));
                    return;
				}
				tkc->keys[tkc->nb_keys].hls_info = gf_strdup(att->value);
			}
			else if (!stricmp(att->name, "IV_size")) {
				IV_size = atoi(att->value);
			}
			else if (!stricmp(att->name, "constant_IV")) {
				char *sKey = att->value;
				if (!strnicmp(sKey, "0x", 2)) sKey += 2;
				if ((strlen(sKey) == 16) || (strlen(sKey) == 32)) {
					u32 j;
					for (j=0; j<strlen(sKey); j+=2) {
						u32 v;
						char szV[5];
						sprintf(szV, "%c%c", sKey[j], sKey[j+1]);
						sscanf(szV, "%x", &v);
						tkc->keys[tkc->nb_keys].IV[j/2] = v;
					}
					const_IV_size = (u8) strlen(sKey) / 2;
					kas_civ = GF_TRUE;
				}
			}
		}
		tkc->keys[tkc->nb_keys].IV_size = IV_size;
		tkc->keys[tkc->nb_keys].constant_IV_size = const_IV_size;
		if (!kas_civ && tkc->nb_keys)
			memcpy(tkc->keys[tkc->nb_keys].IV, tkc->keys[0].IV, 16);
		tkc->nb_keys++;
	}
}