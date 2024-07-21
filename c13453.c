static pj_status_t get_name(int rec_counter, const pj_uint8_t *pkt, 
			    const pj_uint8_t *start, const pj_uint8_t *max,
			    pj_str_t *name)
{
    const pj_uint8_t *p;
    pj_status_t status;

    /* Limit the number of recursion */
    if (rec_counter > 10) {
	/* Too many name recursion */
	return PJLIB_UTIL_EDNSINNAMEPTR;
    }

    p = start;
    while (*p) {
	if ((*p & 0xc0) == 0xc0) {
	    /* Compression is found! */
	    pj_uint16_t offset;

	    /* Get the 14bit offset */
	    pj_memcpy(&offset, p, 2);
	    offset ^= pj_htons((pj_uint16_t)(0xc0 << 8));
	    offset = pj_ntohs(offset);

	    /* Check that offset is valid */
	    if (offset >= max - pkt)
		return PJLIB_UTIL_EDNSINNAMEPTR;

	    /* Retrieve the name from that offset. */
	    status = get_name(rec_counter+1, pkt, pkt + offset, max, name);
	    if (status != PJ_SUCCESS)
		return status;

	    return PJ_SUCCESS;
	} else {
	    unsigned label_len = *p;

	    /* Check that label length is valid */
	    if (pkt+label_len > max)
		return PJLIB_UTIL_EDNSINNAMEPTR;

	    pj_memcpy(name->ptr + name->slen, p+1, label_len);
	    name->slen += label_len;

	    p += label_len + 1;
	    if (*p != 0) {
		*(name->ptr + name->slen) = '.';
		++name->slen;
	    }

	    if (p >= max)
		return PJLIB_UTIL_EDNSINSIZE;
	}
    }

    return PJ_SUCCESS;
}