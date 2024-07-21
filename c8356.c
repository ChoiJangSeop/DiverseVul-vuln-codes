int h2_make_htx_response(struct http_hdr *list, struct htx *htx, unsigned int *msgf, unsigned long long *body_len)
{
	struct ist phdr_val[H2_PHDR_NUM_ENTRIES];
	uint32_t fields; /* bit mask of H2_PHDR_FND_* */
	uint32_t idx;
	int phdr;
	int ret;
	int i;
	uint32_t used = htx_used_space(htx);
	struct htx_sl *sl = NULL;
	unsigned int sl_flags = 0;

	fields = 0;
	for (idx = 0; list[idx].n.len != 0; idx++) {
		if (!list[idx].n.ptr) {
			/* this is an indexed pseudo-header */
			phdr = list[idx].n.len;
		}
		else {
			/* this can be any type of header */
			/* RFC7540#8.1.2: upper case not allowed in header field names */
			for (i = 0; i < list[idx].n.len; i++)
				if ((uint8_t)(list[idx].n.ptr[i] - 'A') < 'Z' - 'A')
					goto fail;

			phdr = h2_str_to_phdr(list[idx].n);
		}

		if (phdr > 0 && phdr < H2_PHDR_NUM_ENTRIES) {
			/* insert a pseudo header by its index (in phdr) and value (in value) */
			if (fields & ((1 << phdr) | H2_PHDR_FND_NONE)) {
				if (fields & H2_PHDR_FND_NONE) {
					/* pseudo header field after regular headers */
					goto fail;
				}
				else {
					/* repeated pseudo header field */
					goto fail;
				}
			}
			fields |= 1 << phdr;
			phdr_val[phdr] = list[idx].v;
			continue;
		}
		else if (phdr != 0) {
			/* invalid pseudo header -- should never happen here */
			goto fail;
		}

		/* regular header field in (name,value) */
		if (!(fields & H2_PHDR_FND_NONE)) {
			/* no more pseudo-headers, time to build the status line */
			sl = h2_prepare_htx_stsline(fields, phdr_val, htx, msgf);
			if (!sl)
				goto fail;
			fields |= H2_PHDR_FND_NONE;
		}

		if (isteq(list[idx].n, ist("content-length"))) {
			ret = h2_parse_cont_len_header(msgf, &list[idx].v, body_len);
			if (ret < 0)
				goto fail;

			sl_flags |= HTX_SL_F_CLEN;
			if (ret == 0)
				continue; // skip this duplicate
		}

		/* these ones are forbidden in responses (RFC7540#8.1.2.2) */
		if (isteq(list[idx].n, ist("connection")) ||
		    isteq(list[idx].n, ist("proxy-connection")) ||
		    isteq(list[idx].n, ist("keep-alive")) ||
		    isteq(list[idx].n, ist("upgrade")) ||
		    isteq(list[idx].n, ist("transfer-encoding")))
			goto fail;

		if (!htx_add_header(htx, list[idx].n, list[idx].v))
			goto fail;
	}

	/* RFC7540#8.1.2.1 mandates to reject request pseudo-headers */
	if (fields & (H2_PHDR_FND_AUTH|H2_PHDR_FND_METH|H2_PHDR_FND_PATH|H2_PHDR_FND_SCHM))
		goto fail;

	/* Let's dump the request now if not yet emitted. */
	if (!(fields & H2_PHDR_FND_NONE)) {
		sl = h2_prepare_htx_stsline(fields, phdr_val, htx, msgf);
		if (!sl)
			goto fail;
	}

	if (!(*msgf & H2_MSGF_BODY) || ((*msgf & H2_MSGF_BODY_CL) && *body_len == 0))
		sl_flags |= HTX_SL_F_BODYLESS;

	/* update the start line with last detected header info */
	sl->flags |= sl_flags;

	if ((*msgf & (H2_MSGF_BODY|H2_MSGF_BODY_TUNNEL|H2_MSGF_BODY_CL)) == H2_MSGF_BODY) {
		/* FIXME: Do we need to signal anything when we have a body and
		 * no content-length, to have the equivalent of H1's chunked
		 * encoding?
		 */
	}

	/* now send the end of headers marker */
	htx_add_endof(htx, HTX_BLK_EOH);

	/* Set bytes used in the HTX mesage for the headers now */
	sl->hdrs_bytes = htx_used_space(htx) - used;

	ret = 1;
	return ret;

 fail:
	return -1;
}