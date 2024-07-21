delegpt_add_target(struct delegpt* dp, struct regional* region, 
	uint8_t* name, size_t namelen, struct sockaddr_storage* addr, 
	socklen_t addrlen, uint8_t bogus, uint8_t lame)
{
	struct delegpt_ns* ns = delegpt_find_ns(dp, name, namelen);
	log_assert(!dp->dp_type_mlc);
	if(!ns) {
		/* ignore it */
		return 1;
	}
	if(!lame) {
		if(addr_is_ip6(addr, addrlen))
			ns->got6 = 1;
		else	ns->got4 = 1;
		if(ns->got4 && ns->got6)
			ns->resolved = 1;
	}
	return delegpt_add_addr(dp, region, addr, addrlen, bogus, lame, NULL);
}