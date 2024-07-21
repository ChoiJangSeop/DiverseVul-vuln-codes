delegpt_add_rrset(struct delegpt* dp, struct regional* region,
        struct ub_packed_rrset_key* rrset, uint8_t lame)
{
	if(!rrset)
		return 1;
	if(ntohs(rrset->rk.type) == LDNS_RR_TYPE_NS)
		return delegpt_rrset_add_ns(dp, region, rrset, lame);
	else if(ntohs(rrset->rk.type) == LDNS_RR_TYPE_A)
		return delegpt_add_rrset_A(dp, region, rrset, lame);
	else if(ntohs(rrset->rk.type) == LDNS_RR_TYPE_AAAA)
		return delegpt_add_rrset_AAAA(dp, region, rrset, lame);
	log_warn("Unknown rrset type added to delegpt");
	return 1;
}