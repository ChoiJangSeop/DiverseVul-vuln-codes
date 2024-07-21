cache_fill_missing(struct module_env* env, uint16_t qclass, 
	struct regional* region, struct delegpt* dp)
{
	struct delegpt_ns* ns;
	struct msgreply_entry* neg;
	struct ub_packed_rrset_key* akey;
	time_t now = *env->now;
	for(ns = dp->nslist; ns; ns = ns->next) {
		akey = rrset_cache_lookup(env->rrset_cache, ns->name, 
			ns->namelen, LDNS_RR_TYPE_A, qclass, 0, now, 0);
		if(akey) {
			if(!delegpt_add_rrset_A(dp, region, akey, ns->lame)) {
				lock_rw_unlock(&akey->entry.lock);
				return 0;
			}
			log_nametypeclass(VERB_ALGO, "found in cache",
				ns->name, LDNS_RR_TYPE_A, qclass);
			lock_rw_unlock(&akey->entry.lock);
		} else {
			/* BIT_CD on false because delegpt lookup does
			 * not use dns64 translation */
			neg = msg_cache_lookup(env, ns->name, ns->namelen,
				LDNS_RR_TYPE_A, qclass, 0, now, 0);
			if(neg) {
				delegpt_add_neg_msg(dp, neg);
				lock_rw_unlock(&neg->entry.lock);
			}
		}
		akey = rrset_cache_lookup(env->rrset_cache, ns->name, 
			ns->namelen, LDNS_RR_TYPE_AAAA, qclass, 0, now, 0);
		if(akey) {
			if(!delegpt_add_rrset_AAAA(dp, region, akey, ns->lame)) {
				lock_rw_unlock(&akey->entry.lock);
				return 0;
			}
			log_nametypeclass(VERB_ALGO, "found in cache",
				ns->name, LDNS_RR_TYPE_AAAA, qclass);
			lock_rw_unlock(&akey->entry.lock);
		} else {
			/* BIT_CD on false because delegpt lookup does
			 * not use dns64 translation */
			neg = msg_cache_lookup(env, ns->name, ns->namelen,
				LDNS_RR_TYPE_AAAA, qclass, 0, now, 0);
			if(neg) {
				delegpt_add_neg_msg(dp, neg);
				lock_rw_unlock(&neg->entry.lock);
			}
		}
	}
	return 1;
}