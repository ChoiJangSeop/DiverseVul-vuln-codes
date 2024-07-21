dname_pkt_hash(sldns_buffer* pkt, uint8_t* dname, hashvalue_type h)
{
	uint8_t labuf[LDNS_MAX_LABELLEN+1];
	uint8_t lablen;
	int i;

	/* preserve case of query, make hash label by label */
	lablen = *dname++;
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			dname = sldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		log_assert(lablen <= LDNS_MAX_LABELLEN);
		labuf[0] = lablen;
		i=0;
		while(lablen--) {
			labuf[++i] = (uint8_t)tolower((unsigned char)*dname);
			dname++;
		}
		h = hashlittle(labuf, labuf[0] + 1, h);
		lablen = *dname++;
	}

	return h;
}