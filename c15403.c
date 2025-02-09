iasecc_process_fci(struct sc_card *card, struct sc_file *file,
		 const unsigned char *buf, size_t buflen)
{
	struct sc_context *ctx = card->ctx;
	size_t taglen;
	int rv, ii, offs;
	const unsigned char *acls = NULL, *tag = NULL;
	unsigned char mask;
	unsigned char ops_DF[7] = {
		SC_AC_OP_DELETE, 0xFF, SC_AC_OP_ACTIVATE, SC_AC_OP_DEACTIVATE, 0xFF, SC_AC_OP_CREATE, 0xFF
	};
	unsigned char ops_EF[7] = {
		SC_AC_OP_DELETE, 0xFF, SC_AC_OP_ACTIVATE, SC_AC_OP_DEACTIVATE, 0xFF, SC_AC_OP_UPDATE, SC_AC_OP_READ
	};

	LOG_FUNC_CALLED(ctx);

	tag = sc_asn1_find_tag(ctx,  buf, buflen, 0x6F, &taglen);
	sc_log(ctx, "processing FCI: 0x6F tag %p", tag);
	if (tag != NULL) {
		sc_log(ctx, "  FCP length %"SC_FORMAT_LEN_SIZE_T"u", taglen);
		buf = tag;
		buflen = taglen;
	}

	tag = sc_asn1_find_tag(ctx,  buf, buflen, 0x62, &taglen);
	sc_log(ctx, "processing FCI: 0x62 tag %p", tag);
	if (tag != NULL) {
		sc_log(ctx, "  FCP length %"SC_FORMAT_LEN_SIZE_T"u", taglen);
		buf = tag;
		buflen = taglen;
	}

	rv = iso_ops->process_fci(card, file, buf, buflen);
	LOG_TEST_RET(ctx, rv, "ISO parse FCI failed");
/*
	Gemalto:  6F 19 80 02 02 ED 82 01 01 83 02 B0 01 88 00	8C 07 7B 17 17 17 17 17 00 8A 01 05 90 00
	Sagem:    6F 17 62 15 80 02 00 7D 82 01 01                   8C 02 01 00 83 02 2F 00 88 01 F0 8A 01 05 90 00
	Oberthur: 62 1B 80 02 05 DC 82 01 01 83 02 B0 01 88 00 A1 09 8C 07 7B 17 FF 17 17 17 00 8A 01 05 90 00
*/

	sc_log(ctx, "iasecc_process_fci() type %i; let's parse file ACLs", file->type);
	tag = sc_asn1_find_tag(ctx, buf, buflen, IASECC_DOCP_TAG_ACLS, &taglen);
	if (tag)
		acls = sc_asn1_find_tag(ctx, tag, taglen, IASECC_DOCP_TAG_ACLS_CONTACT, &taglen);
	else
		acls = sc_asn1_find_tag(ctx, buf, buflen, IASECC_DOCP_TAG_ACLS_CONTACT, &taglen);

	if (!acls)   {
		sc_log(ctx,
		       "ACLs not found in data(%"SC_FORMAT_LEN_SIZE_T"u) %s",
		       buflen, sc_dump_hex(buf, buflen));
		LOG_TEST_RET(ctx, SC_ERROR_OBJECT_NOT_FOUND, "ACLs tag missing");
	}

	sc_log(ctx, "ACLs(%"SC_FORMAT_LEN_SIZE_T"u) '%s'", taglen,
	       sc_dump_hex(acls, taglen));
	mask = 0x40, offs = 1;
	for (ii = 0; ii < 7; ii++, mask /= 2)  {
		unsigned char op = file->type == SC_FILE_TYPE_DF ? ops_DF[ii] : ops_EF[ii];

		if (!(mask & acls[0]))
			continue;

		sc_log(ctx, "ACLs mask 0x%X, offs %i, op 0x%X, acls[offs] 0x%X", mask, offs, op, acls[offs]);
		if (op == 0xFF)   {
			;
		}
		else if (acls[offs] == 0)   {
			sc_file_add_acl_entry(file, op, SC_AC_NONE, 0);
		}
		else if (acls[offs] == 0xFF)   {
			sc_file_add_acl_entry(file, op, SC_AC_NEVER, 0);
		}
		else if ((acls[offs] & IASECC_SCB_METHOD_MASK) == IASECC_SCB_METHOD_USER_AUTH)   {
			sc_file_add_acl_entry(file, op, SC_AC_SEN, acls[offs] & IASECC_SCB_METHOD_MASK_REF);
		}
		else if (acls[offs] & IASECC_SCB_METHOD_MASK)   {
			sc_file_add_acl_entry(file, op, SC_AC_SCB, acls[offs]);
		}
		else   {
			sc_log(ctx, "Warning: non supported SCB method: %X", acls[offs]);
			sc_file_add_acl_entry(file, op, SC_AC_NEVER, 0);
		}

		offs++;
	}

	LOG_FUNC_RETURN(ctx, 0);
}