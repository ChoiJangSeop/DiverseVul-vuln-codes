sc_pkcs15emu_oberthur_add_prvkey(struct sc_pkcs15_card *p15card,
		unsigned int file_id, unsigned int size)
{
	struct sc_context *ctx = p15card->card->ctx;
	struct sc_pkcs15_prkey_info kinfo;
	struct sc_pkcs15_object kobj;
	struct crypto_container ccont;
	unsigned char *info_blob = NULL;
	size_t info_len = 0;
	unsigned flags;
	size_t offs, len;
	char ch_tmp[0x100];
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "add private key(file-id:%04X,size:%04X)", file_id, size);

	memset(&kinfo, 0, sizeof(kinfo));
	memset(&kobj, 0, sizeof(kobj));
	memset(&ccont, 0, sizeof(ccont));

	rv = sc_oberthur_get_friends (file_id, &ccont);
	LOG_TEST_RET(ctx, rv, "Failed to add private key: get friends error");

	if (ccont.id_cert)   {
		struct sc_pkcs15_object *objs[32];
		int ii;

		sc_log(ctx, "friend certificate %04X", ccont.id_cert);
		rv = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_CERT_X509, objs, 32);
		LOG_TEST_RET(ctx, rv, "Failed to add private key: get certificates error");

		for (ii=0; ii<rv; ii++) {
			struct sc_pkcs15_cert_info *cert = (struct sc_pkcs15_cert_info *)objs[ii]->data;
			struct sc_path path = cert->path;
			unsigned int id = path.value[path.len - 2] * 0x100 + path.value[path.len - 1];

			if (id == ccont.id_cert)   {
				strlcpy(kobj.label, objs[ii]->label, sizeof(kobj.label));
				break;
			}
		}

		if (ii == rv)
			LOG_TEST_RET(ctx, SC_ERROR_INCONSISTENT_PROFILE, "Failed to add private key: friend not found");
	}

	snprintf(ch_tmp, sizeof(ch_tmp), "%s%04X", AWP_OBJECTS_DF_PRV, file_id | 0x100);
	rv = sc_oberthur_read_file(p15card, ch_tmp, &info_blob, &info_len, 1);
	LOG_TEST_RET(ctx, rv, "Failed to add private key: read oberthur file error");

	if (info_len < 2)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add private key: no 'tag'");
	flags = *(info_blob + 0) * 0x100 + *(info_blob + 1);
	offs = 2;

	/* CN */
	if (offs > info_len)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add private key: no 'CN'");
	len = *(info_blob + offs + 1) + *(info_blob + offs) * 0x100;
	if (len && !strlen(kobj.label))   {
		if (len > sizeof(kobj.label) - 1)
			len = sizeof(kobj.label) - 1;
		strncpy(kobj.label, (char *)(info_blob + offs + 2), len);
	}
	offs += 2 + len;

	/* ID */
	if (offs > info_len)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add private key: no 'ID'");
	len = *(info_blob + offs + 1) + *(info_blob + offs) * 0x100;
	if (!len)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add private key: zero length ID");
	else if (len > sizeof(kinfo.id.value))
		LOG_TEST_RET(ctx, SC_ERROR_INVALID_DATA, "Failed to add private key: invalid ID length");
	memcpy(kinfo.id.value, info_blob + offs + 2, len);
	kinfo.id.len = len;
	offs += 2 + len;

	/* Ignore Start/End dates */
	offs += 16;

	/* Subject encoded in ASN1 */
	if (offs > info_len)
		return SC_ERROR_UNKNOWN_DATA_RECEIVED;
	len = *(info_blob + offs + 1) + *(info_blob + offs) * 0x100;
	if (len)   {
		kinfo.subject.value = malloc(len);
		if (!kinfo.subject.value)
			LOG_TEST_RET(ctx, SC_ERROR_OUT_OF_MEMORY, "Failed to add private key: memory allocation error");
		kinfo.subject.len = len;
		memcpy(kinfo.subject.value, info_blob + offs + 2, len);
	}

	/* Modulus and exponent are ignored */

	snprintf(ch_tmp, sizeof(ch_tmp), "%s%04X", AWP_OBJECTS_DF_PRV, file_id);
	sc_format_path(ch_tmp, &kinfo.path);
	sc_log(ctx, "Private key info path %s", ch_tmp);

	kinfo.modulus_length	= size;
	kinfo.native		= 1;
	kinfo.key_reference	 = file_id & 0xFF;

	kinfo.usage = sc_oberthur_decode_usage(flags);
	kobj.flags = SC_PKCS15_CO_FLAG_PRIVATE;
	if (flags & OBERTHUR_ATTR_MODIFIABLE)
		kobj.flags |= SC_PKCS15_CO_FLAG_MODIFIABLE;

	kobj.auth_id.len = sizeof(PinDomainID) > sizeof(kobj.auth_id.value)
			? sizeof(kobj.auth_id.value) : sizeof(PinDomainID);
	memcpy(kobj.auth_id.value, PinDomainID, kobj.auth_id.len);

	sc_log(ctx, "Parsed private key(reference:%i,usage:%X,flags:%X)", kinfo.key_reference, kinfo.usage, kobj.flags);

	rv = sc_pkcs15emu_add_rsa_prkey(p15card, &kobj, &kinfo);
	LOG_FUNC_RETURN(ctx, rv);
}