void dname_pkt_copy(sldns_buffer* pkt, uint8_t* to, uint8_t* dname)
{
	/* copy over the dname and decompress it at the same time */
	size_t len = 0;
	uint8_t lablen;
	lablen = *dname++;
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			dname = sldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		if(lablen > LDNS_MAX_LABELLEN) {
			*to = 0; /* end the result prematurely */
			return;
		}
		log_assert(lablen <= LDNS_MAX_LABELLEN);
		len += (size_t)lablen+1;
		if(len >= LDNS_MAX_DOMAINLEN) {
			*to = 0; /* end the result prematurely */
			log_err("bad dname in dname_pkt_copy");
			return;
		}
		*to++ = lablen;
		memmove(to, dname, lablen);
		dname += lablen;
		to += lablen;
		lablen = *dname++;
	}
	/* copy last \0 */
	*to = 0;
}