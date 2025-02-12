static int insert_key(
	sc_pkcs15_card_t *p15card,
	const char       *path,
	unsigned char     id,
	unsigned char     key_reference,
	int               key_length,
	unsigned char     auth_id,
	const char       *label
){
	sc_card_t *card=p15card->card;
	sc_context_t *ctx=p15card->card->ctx;
	sc_file_t *f;
	struct sc_pkcs15_prkey_info prkey_info;
	struct sc_pkcs15_object prkey_obj;
	int r, can_sign, can_crypt;

	memset(&prkey_info, 0, sizeof(prkey_info));
	prkey_info.id.len         = 1;
	prkey_info.id.value[0]    = id;
	prkey_info.native         = 1;
	prkey_info.key_reference  = key_reference;
	prkey_info.modulus_length = key_length;
	sc_format_path(path, &prkey_info.path);

	memset(&prkey_obj, 0, sizeof(prkey_obj));
	strlcpy(prkey_obj.label, label, sizeof(prkey_obj.label));
	prkey_obj.flags            = SC_PKCS15_CO_FLAG_PRIVATE;
	prkey_obj.auth_id.len      = 1;
	prkey_obj.auth_id.value[0] = auth_id;

	can_sign=can_crypt=0;
	if(card->type==SC_CARD_TYPE_TCOS_V3){
		unsigned char buf[256];
		int i, rec_no=0;
		if(prkey_info.path.len>=2) prkey_info.path.len-=2;
		sc_append_file_id(&prkey_info.path, 0x5349);
		if(sc_select_file(card, &prkey_info.path, NULL)!=SC_SUCCESS){
			sc_debug(ctx, SC_LOG_DEBUG_NORMAL,
				"Select(%s) failed\n",
				sc_print_path(&prkey_info.path));
			return 1;
		}
		sc_debug(ctx, SC_LOG_DEBUG_NORMAL,
			"Searching for Key-Ref %02X\n", key_reference);
		while((r=sc_read_record(card, ++rec_no, buf, sizeof(buf), SC_RECORD_BY_REC_NR))>0){
			int found=0;
			if(buf[0]!=0xA0) continue;
			for(i=2;i<buf[1]+2;i+=2+buf[i+1]){
				if(buf[i]==0x83 && buf[i+1]==1 && buf[i+2]==key_reference) ++found;
			}
			if(found) break;
		}
		if(r<=0){
			sc_debug(ctx, SC_LOG_DEBUG_NORMAL,"No EF_KEYD-Record found\n");
			return 1;
		}
		for(i=0;i<r;i+=2+buf[i+1]){
			if(buf[i]==0xB6) can_sign++;
			if(buf[i]==0xB8) can_crypt++;
		}
	} else {
		if(sc_select_file(card, &prkey_info.path, &f)!=SC_SUCCESS){
			sc_debug(ctx, SC_LOG_DEBUG_NORMAL,
				"Select(%s) failed\n",
				sc_print_path(&prkey_info.path));
			return 1;
		}
		if (f->prop_attr[1] & 0x04) can_crypt=1;
		if (f->prop_attr[1] & 0x08) can_sign=1;
		sc_file_free(f);
	}
	prkey_info.usage= SC_PKCS15_PRKEY_USAGE_SIGN;
	if(can_crypt) prkey_info.usage |= SC_PKCS15_PRKEY_USAGE_ENCRYPT|SC_PKCS15_PRKEY_USAGE_DECRYPT;
	if(can_sign) prkey_info.usage |= SC_PKCS15_PRKEY_USAGE_NONREPUDIATION;

	r=sc_pkcs15emu_add_rsa_prkey(p15card, &prkey_obj, &prkey_info);
	if(r!=SC_SUCCESS){
		sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "sc_pkcs15emu_add_rsa_prkey(%s) failed\n", path);
		return 4;
	}
	sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "%s: OK%s%s\n", path, can_sign ? ", Sign" : "", can_crypt ? ", Crypt" : "");
	return 0;
}