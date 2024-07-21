parse_get_cname_target(struct rrset_parse* rrset, uint8_t** sname, 
	size_t* snamelen)
{
	if(rrset->rr_count != 1) {
		struct rr_parse* sig;
		verbose(VERB_ALGO, "Found CNAME rrset with "
			"size > 1: %u", (unsigned)rrset->rr_count);
		/* use the first CNAME! */
		rrset->rr_count = 1;
		rrset->size = rrset->rr_first->size;
		for(sig=rrset->rrsig_first; sig; sig=sig->next)
			rrset->size += sig->size;
		rrset->rr_last = rrset->rr_first;
		rrset->rr_first->next = NULL;
	}
	if(rrset->rr_first->size < sizeof(uint16_t)+1)
		return 0; /* CNAME rdata too small */
	*sname = rrset->rr_first->ttl_data + sizeof(uint32_t)
		+ sizeof(uint16_t); /* skip ttl, rdatalen */
	*snamelen = rrset->rr_first->size - sizeof(uint16_t);
	return 1;
}