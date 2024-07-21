dns_cache_find_delegation(struct module_env* env, uint8_t* qname, 
	size_t qnamelen, uint16_t qtype, uint16_t qclass, 
	struct regional* region, struct dns_msg** msg, time_t now)
{
	/* try to find closest NS rrset */
	struct ub_packed_rrset_key* nskey;
	struct packed_rrset_data* nsdata;
	struct delegpt* dp;

	nskey = find_closest_of_type(env, qname, qnamelen, qclass, now,
		LDNS_RR_TYPE_NS, 0);
	if(!nskey) /* hope the caller has hints to prime or something */
		return NULL;
	nsdata = (struct packed_rrset_data*)nskey->entry.data;
	/* got the NS key, create delegation point */
	dp = delegpt_create(region);
	if(!dp || !delegpt_set_name(dp, region, nskey->rk.dname)) {
		lock_rw_unlock(&nskey->entry.lock);
		log_err("find_delegation: out of memory");
		return NULL;
	}
	/* create referral message */
	if(msg) {
		/* allocate the array to as much as we could need:
		 *	NS rrset + DS/NSEC rrset +
		 *	A rrset for every NS RR
		 *	AAAA rrset for every NS RR
		 */
		*msg = dns_msg_create(qname, qnamelen, qtype, qclass, region, 
			2 + nsdata->count*2);
		if(!*msg || !dns_msg_authadd(*msg, region, nskey, now)) {
			lock_rw_unlock(&nskey->entry.lock);
			log_err("find_delegation: out of memory");
			return NULL;
		}
	}
	if(!delegpt_rrset_add_ns(dp, region, nskey, 0))
		log_err("find_delegation: addns out of memory");
	lock_rw_unlock(&nskey->entry.lock); /* first unlock before next lookup*/
	/* find and add DS/NSEC (if any) */
	if(msg)
		find_add_ds(env, region, *msg, dp, now);
	/* find and add A entries */
	if(!find_add_addrs(env, qclass, region, dp, now, msg))
		log_err("find_delegation: addrs out of memory");
	return dp;
}