parse_escape(uint8_t *s, uint8_t *q) {
	uint16_t val;
	if (strlen((char *)s) > 3 &&
	    isdigit((int) s[1]) &&
	    isdigit((int) s[2]) &&
	    isdigit((int) s[3])) {
		/* cast this so it fits */
		val = (uint16_t) ldns_hexdigit_to_int((char) s[1]) * 100 +
		                ldns_hexdigit_to_int((char) s[2]) * 10 +
		                ldns_hexdigit_to_int((char) s[3]);
		if (val > 255) {
			/* outside range */
			return 0;
		}
		*q = (uint8_t) val;
                return 3;
	} else {
		s++;
		if (*s == '\0' || isdigit((int) *s)) {
			/* apparently the string terminator
			 * or a digit has been escaped...
		         */
			return 0;
		}
		*q = *s;
		return 1;
	}
}