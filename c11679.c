find_closest_of_type(struct module_env* env, uint8_t* qname, size_t qnamelen, 
	uint16_t qclass, time_t now, uint16_t searchtype, int stripfront)
{
	struct ub_packed_rrset_key *rrset;
	uint8_t lablen;

	if(stripfront) {
		/* strip off so that DNAMEs have strict subdomain match */
		lablen = *qname;
		qname += lablen + 1;
		qnamelen -= lablen + 1;
	}

	/* snip off front part of qname until the type is found */
	while(qnamelen > 0) {
		if((rrset = rrset_cache_lookup(env->rrset_cache, qname, 
			qnamelen, searchtype, qclass, 0, now, 0)))
			return rrset;

		/* snip off front label */
		lablen = *qname;
		qname += lablen + 1;
		qnamelen -= lablen + 1;
	}
	return NULL;
}