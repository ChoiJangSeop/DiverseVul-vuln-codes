int iter_lookup_parent_glue_from_cache(struct module_env* env,
        struct delegpt* dp, struct regional* region, struct query_info* qinfo)
{
	struct ub_packed_rrset_key* akey;
	struct delegpt_ns* ns;
	size_t num = delegpt_count_targets(dp);
	for(ns = dp->nslist; ns; ns = ns->next) {
		/* get cached parentside A */
		akey = rrset_cache_lookup(env->rrset_cache, ns->name, 
			ns->namelen, LDNS_RR_TYPE_A, qinfo->qclass, 
			PACKED_RRSET_PARENT_SIDE, *env->now, 0);
		if(akey) {
			log_rrset_key(VERB_ALGO, "found parent-side", akey);
			ns->done_pside4 = 1;
			/* a negative-cache-element has no addresses it adds */
			if(!delegpt_add_rrset_A(dp, region, akey, 1))
				log_err("malloc failure in lookup_parent_glue");
			lock_rw_unlock(&akey->entry.lock);
		}
		/* get cached parentside AAAA */
		akey = rrset_cache_lookup(env->rrset_cache, ns->name, 
			ns->namelen, LDNS_RR_TYPE_AAAA, qinfo->qclass, 
			PACKED_RRSET_PARENT_SIDE, *env->now, 0);
		if(akey) {
			log_rrset_key(VERB_ALGO, "found parent-side", akey);
			ns->done_pside6 = 1;
			/* a negative-cache-element has no addresses it adds */
			if(!delegpt_add_rrset_AAAA(dp, region, akey, 1))
				log_err("malloc failure in lookup_parent_glue");
			lock_rw_unlock(&akey->entry.lock);
		}
	}
	/* see if new (but lame) addresses have become available */
	return delegpt_count_targets(dp) != num;
}