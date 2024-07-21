sc_pkcs15emu_oberthur_add_cert(struct sc_pkcs15_card *p15card, unsigned int file_id)
{
	struct sc_context *ctx = p15card->card->ctx;
	struct sc_pkcs15_cert_info cinfo;
	struct sc_pkcs15_object cobj;
	unsigned char *info_blob, *cert_blob;
	size_t info_len, cert_len, len, offs;
	unsigned flags;
	int rv;
	char ch_tmp[0x20];

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "add certificate(file-id:%04X)", file_id);

	memset(&cinfo, 0, sizeof(cinfo));
	memset(&cobj, 0, sizeof(cobj));

	snprintf(ch_tmp, sizeof(ch_tmp), "%s%04X", AWP_OBJECTS_DF_PUB, file_id | 0x100);
	rv = sc_oberthur_read_file(p15card, ch_tmp, &info_blob, &info_len, 1);
	LOG_TEST_RET(ctx, rv, "Failed to add certificate: read oberthur file error");

	if (info_len < 2)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add certificate: no 'tag'");
	flags = *(info_blob + 0) * 0x100 + *(info_blob + 1);
	offs = 2;

	/* Label */
	if (offs + 2 > info_len)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add certificate: no 'CN'");
	len = *(info_blob + offs + 1) + *(info_blob + offs) * 0x100;
	if (len)   {
		if (len > sizeof(cobj.label) - 1)
			len = sizeof(cobj.label) - 1;
		memcpy(cobj.label, info_blob + offs + 2, len);
	}
	offs += 2 + len;

	/* ID */
	if (offs > info_len)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "Failed to add certificate: no 'ID'");
	len = *(info_blob + offs + 1) + *(info_blob + offs) * 0x100;
	if (len > sizeof(cinfo.id.value))
		LOG_TEST_RET(ctx, SC_ERROR_INVALID_DATA, "Failed to add certificate: invalid 'ID' length");
	memcpy(cinfo.id.value, info_blob + offs + 2, len);
	cinfo.id.len = len;

	/* Ignore subject, issuer and serial */

	snprintf(ch_tmp, sizeof(ch_tmp), "%s%04X", AWP_OBJECTS_DF_PUB, file_id);
	sc_format_path(ch_tmp, &cinfo.path);
	rv = sc_oberthur_read_file(p15card, ch_tmp, &cert_blob, &cert_len, 1);
	LOG_TEST_RET(ctx, rv, "Failed to add certificate: read certificate error");

	cinfo.value.value = cert_blob;
	cinfo.value.len = cert_len;

	rv = sc_oberthur_get_certificate_authority(&cinfo.value, &cinfo.authority);
	LOG_TEST_RET(ctx, rv, "Failed to add certificate: get certificate attributes error");

	if (flags & OBERTHUR_ATTR_MODIFIABLE)
		cobj.flags |= SC_PKCS15_CO_FLAG_MODIFIABLE;

	rv = sc_pkcs15emu_add_x509_cert(p15card, &cobj, &cinfo);

	LOG_FUNC_RETURN(p15card->card->ctx, rv);
}