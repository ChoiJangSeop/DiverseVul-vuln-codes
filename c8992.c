delegpt_add_addr(struct delegpt* dp, struct regional* region, 
	struct sockaddr_storage* addr, socklen_t addrlen, uint8_t bogus, 
	uint8_t lame, char* tls_auth_name)
{
	struct delegpt_addr* a;
	log_assert(!dp->dp_type_mlc);
	/* check for duplicates */
	if((a = delegpt_find_addr(dp, addr, addrlen))) {
		if(bogus)
			a->bogus = bogus;
		if(!lame)
			a->lame = 0;
		return 1;
	}

	a = (struct delegpt_addr*)regional_alloc(region,
		sizeof(struct delegpt_addr));
	if(!a)
		return 0;
	a->next_target = dp->target_list;
	dp->target_list = a;
	a->next_result = 0;
	a->next_usable = dp->usable_list;
	dp->usable_list = a;
	memcpy(&a->addr, addr, addrlen);
	a->addrlen = addrlen;
	a->attempts = 0;
	a->bogus = bogus;
	a->lame = lame;
	a->dnsseclame = 0;
	if(tls_auth_name) {
		a->tls_auth_name = regional_strdup(region, tls_auth_name);
		if(!a->tls_auth_name)
			return 0;
	} else {
		a->tls_auth_name = NULL;
	}
	return 1;
}