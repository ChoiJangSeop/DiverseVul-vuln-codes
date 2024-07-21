ldns_str2rdf_str(ldns_rdf **rd, const char *str)
{
	uint8_t *data;
	size_t i, str_i, esc_i;

	if (strlen(str) > 255) {
		return LDNS_STATUS_INVALID_STR;
	}

	data = LDNS_XMALLOC(uint8_t, strlen(str) + 1);
        if(!data) return LDNS_STATUS_MEM_ERR;
	i = 1;

	for (str_i = 0; str_i < strlen(str); str_i++) {
		if (str[str_i] == '\\') {
			/* octet value or literal char */
			esc_i = (size_t) parse_escape((uint8_t*) &str[str_i], (uint8_t*) &data[i]);
			if (esc_i == 0) {
				LDNS_FREE(data);
				return LDNS_STATUS_SYNTAX_BAD_ESCAPE;
			}
			str_i += esc_i;
		} else {
			data[i] = (uint8_t) str[str_i];
		}
		i++;
	}
	data[0] = i - 1;
	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_STR, i, data);

	LDNS_FREE(data);
	return *rd?LDNS_STATUS_OK:LDNS_STATUS_MEM_ERR;
}