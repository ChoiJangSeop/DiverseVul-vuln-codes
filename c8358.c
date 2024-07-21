int h2_make_htx_trailers(struct http_hdr *list, struct htx *htx)
{
	const char *ctl;
	uint32_t idx;
	int i;

	for (idx = 0; list[idx].n.len != 0; idx++) {
		if (!list[idx].n.ptr) {
			/* This is an indexed pseudo-header (RFC7540#8.1.2.1) */
			goto fail;
		}

		/* RFC7540#8.1.2: upper case not allowed in header field names */
		for (i = 0; i < list[idx].n.len; i++)
			if ((uint8_t)(list[idx].n.ptr[i] - 'A') < 'Z' - 'A')
				goto fail;

		if (h2_str_to_phdr(list[idx].n) != 0) {
			/* This is a pseudo-header (RFC7540#8.1.2.1) */
			goto fail;
		}

		/* these ones are forbidden in trailers (RFC7540#8.1.2.2) */
		if (isteq(list[idx].n, ist("host")) ||
		    isteq(list[idx].n, ist("content-length")) ||
		    isteq(list[idx].n, ist("connection")) ||
		    isteq(list[idx].n, ist("proxy-connection")) ||
		    isteq(list[idx].n, ist("keep-alive")) ||
		    isteq(list[idx].n, ist("upgrade")) ||
		    isteq(list[idx].n, ist("te")) ||
		    isteq(list[idx].n, ist("transfer-encoding")))
			goto fail;

		/* RFC7540#10.3: intermediaries forwarding to HTTP/1 must take care of
		 * rejecting NUL, CR and LF characters.
		 */
		ctl = ist_find_ctl(list[idx].v);
		if (unlikely(ctl) && has_forbidden_char(list[idx].v, ctl))
			goto fail;

		if (!htx_add_trailer(htx, list[idx].n, list[idx].v))
			goto fail;
	}

	if (!htx_add_endof(htx, HTX_BLK_EOT))
		goto fail;

	return 1;

 fail:
	return -1;
}