struct delegpt* delegpt_copy(struct delegpt* dp, struct regional* region)
{
	struct delegpt* copy = delegpt_create(region);
	struct delegpt_ns* ns;
	struct delegpt_addr* a;
	if(!copy) 
		return NULL;
	if(!delegpt_set_name(copy, region, dp->name))
		return NULL;
	copy->bogus = dp->bogus;
	copy->has_parent_side_NS = dp->has_parent_side_NS;
	copy->ssl_upstream = dp->ssl_upstream;
	for(ns = dp->nslist; ns; ns = ns->next) {
		if(!delegpt_add_ns(copy, region, ns->name, ns->lame))
			return NULL;
		copy->nslist->resolved = ns->resolved;
		copy->nslist->got4 = ns->got4;
		copy->nslist->got6 = ns->got6;
		copy->nslist->done_pside4 = ns->done_pside4;
		copy->nslist->done_pside6 = ns->done_pside6;
	}
	for(a = dp->target_list; a; a = a->next_target) {
		if(!delegpt_add_addr(copy, region, &a->addr, a->addrlen, 
			a->bogus, a->lame, a->tls_auth_name))
			return NULL;
	}
	return copy;
}