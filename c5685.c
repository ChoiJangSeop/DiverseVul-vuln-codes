ldns_str2rdf_long_str(ldns_rdf **rd, const char *str)
{
	uint8_t *data, *dp, ch = 0;
	size_t length;

	/* Worst case space requirement. We'll realloc to actual size later. */
	dp = data = LDNS_XMALLOC(uint8_t, strlen(str));
        if (! data) {
		return LDNS_STATUS_MEM_ERR;
	}

	/* Fill data with parsed bytes */
	while (parse_char(&ch, &str)) {
		*dp++ = ch;
		if (dp - data > LDNS_MAX_RDFLEN) {
			LDNS_FREE(data);
			return LDNS_STATUS_INVALID_STR;
		}
	}
	if (! str) {
		return LDNS_STATUS_SYNTAX_BAD_ESCAPE;
	}
	length = (size_t)(dp - data);

	/* Lose the overmeasure */
	data = LDNS_XREALLOC(dp = data, uint8_t, length);
	if (! data) {
		LDNS_FREE(dp);
		return LDNS_STATUS_MEM_ERR;
	}

	/* Create rdf */
	*rd = ldns_rdf_new(LDNS_RDF_TYPE_LONG_STR, length, data);
	if (! *rd) {
		LDNS_FREE(data);
		return LDNS_STATUS_MEM_ERR;
	}
	return LDNS_STATUS_OK;
}