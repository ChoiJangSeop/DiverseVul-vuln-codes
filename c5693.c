ldns_str2rdf_dname(ldns_rdf **d, const char *str)
{
	size_t len;

	int esc;
	uint8_t *s, *q, *pq, label_len;
	uint8_t buf[LDNS_MAX_DOMAINLEN + 1];
	*d = NULL;

	len = strlen((char*)str);
	/* octet representation can make strings a lot longer than actual length */
	if (len > LDNS_MAX_DOMAINLEN * 4) {
		return LDNS_STATUS_DOMAINNAME_OVERFLOW;
	}
	if (0 == len) {
		return LDNS_STATUS_DOMAINNAME_UNDERFLOW;
	}

	/* root label */
	if (1 == len && *str == '.') {
		*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, 1, "\0");
		return LDNS_STATUS_OK;
	}

	/* get on with the rest */

	/* s is on the current character in the string
         * pq points to where the labellength is going to go
         * label_len keeps track of the current label's length
	 * q builds the dname inside the buf array
	 */
	len = 0;
	q = buf+1;
	pq = buf;
	label_len = 0;
	for (s = (uint8_t *)str; *s; s++, q++) {
		if (q > buf + LDNS_MAX_DOMAINLEN) {
			return LDNS_STATUS_DOMAINNAME_OVERFLOW;
		}
		*q = 0;
		switch (*s) {
		case '.':
			if (label_len > LDNS_MAX_LABELLEN) {
				return LDNS_STATUS_LABEL_OVERFLOW;
			}
			if (label_len == 0) {
				return LDNS_STATUS_EMPTY_LABEL;
			}
			len += label_len + 1;
			*pq = label_len;
			label_len = 0;
			pq = q;
			break;
		case '\\':
			/* octet value or literal char */
			esc = parse_escape(s, q);
			if (esc > 0) {
				s += esc;
				label_len++;
			} else {
				return LDNS_STATUS_SYNTAX_BAD_ESCAPE;
			}
			break;
		default:
			*q = *s;
			label_len++;
		}
	}

	/* add root label if last char was not '.' */
	if (!ldns_dname_str_absolute(str)) {
		if (q > buf + LDNS_MAX_DOMAINLEN) {
			return LDNS_STATUS_DOMAINNAME_OVERFLOW;
		}
                if (label_len > LDNS_MAX_LABELLEN) {
                        return LDNS_STATUS_LABEL_OVERFLOW;
                }
                if (label_len == 0) { /* label_len 0 but not . at end? */
                        return LDNS_STATUS_EMPTY_LABEL;
                }
		len += label_len + 1;
		*pq = label_len;
		*q = 0;
	}
	len++;

	*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, len, buf);
	return LDNS_STATUS_OK;
}