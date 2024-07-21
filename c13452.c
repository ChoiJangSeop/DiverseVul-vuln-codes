static pj_status_t get_name_len(int rec_counter, const pj_uint8_t *pkt, 
				const pj_uint8_t *start, const pj_uint8_t *max, 
				int *parsed_len, int *name_len)
{
    const pj_uint8_t *p;
    pj_status_t status;

    /* Limit the number of recursion */
    if (rec_counter > 10) {
	/* Too many name recursion */
	return PJLIB_UTIL_EDNSINNAMEPTR;
    }

    *name_len = *parsed_len = 0;
    p = start;
    while (*p) {
	if ((*p & 0xc0) == 0xc0) {
	    /* Compression is found! */
	    int ptr_len = 0;
	    int dummy;
	    pj_uint16_t offset;

	    /* Get the 14bit offset */
	    pj_memcpy(&offset, p, 2);
	    offset ^= pj_htons((pj_uint16_t)(0xc0 << 8));
	    offset = pj_ntohs(offset);

	    /* Check that offset is valid */
	    if (offset >= max - pkt)
		return PJLIB_UTIL_EDNSINNAMEPTR;

	    /* Get the name length from that offset. */
	    status = get_name_len(rec_counter+1, pkt, pkt + offset, max, 
				  &dummy, &ptr_len);
	    if (status != PJ_SUCCESS)
		return status;

	    *parsed_len += 2;
	    *name_len += ptr_len;

	    return PJ_SUCCESS;
	} else {
	    unsigned label_len = *p;

	    /* Check that label length is valid */
	    if (pkt+label_len > max)
		return PJLIB_UTIL_EDNSINNAMEPTR;

	    p += (label_len + 1);
	    *parsed_len += (label_len + 1);

	    if (*p != 0)
		++label_len;
	    
	    *name_len += label_len;

	    if (p >= max)
		return PJLIB_UTIL_EDNSINSIZE;
	}
    }
    ++p;
    (*parsed_len)++;

    return PJ_SUCCESS;
}