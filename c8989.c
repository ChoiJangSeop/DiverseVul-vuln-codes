delegpt_from_message(struct dns_msg* msg, struct regional* region)
{
	struct ub_packed_rrset_key* ns_rrset = NULL;
	struct delegpt* dp;
	size_t i;
	/* look for NS records in the authority section... */
	ns_rrset = find_NS(msg->rep, msg->rep->an_numrrsets, 
		msg->rep->an_numrrsets+msg->rep->ns_numrrsets);

	/* In some cases (even legitimate, perfectly legal cases), the 
	 * NS set for the "referral" might be in the answer section. */
	if(!ns_rrset)
		ns_rrset = find_NS(msg->rep, 0, msg->rep->an_numrrsets);
	
	/* If there was no NS rrset in the authority section, then this 
	 * wasn't a referral message. (It might not actually be a 
	 * referral message anyway) */
	if(!ns_rrset)
		return NULL;
	
	/* If we found any, then Yay! we have a delegation point. */
	dp = delegpt_create(region);
	if(!dp)
		return NULL;
	dp->has_parent_side_NS = 1; /* created from message */
	if(!delegpt_set_name(dp, region, ns_rrset->rk.dname))
		return NULL;
	if(!delegpt_rrset_add_ns(dp, region, ns_rrset, 0))
		return NULL;

	/* add glue, A and AAAA in answer and additional section */
	for(i=0; i<msg->rep->rrset_count; i++) {
		struct ub_packed_rrset_key* s = msg->rep->rrsets[i];
		/* skip auth section. FIXME really needed?*/
		if(msg->rep->an_numrrsets <= i && 
			i < (msg->rep->an_numrrsets+msg->rep->ns_numrrsets))
			continue;

		if(ntohs(s->rk.type) == LDNS_RR_TYPE_A) {
			if(!delegpt_add_rrset_A(dp, region, s, 0))
				return NULL;
		} else if(ntohs(s->rk.type) == LDNS_RR_TYPE_AAAA) {
			if(!delegpt_add_rrset_AAAA(dp, region, s, 0))
				return NULL;
		}
	}
	return dp;
}