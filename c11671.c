dns_cache_store_msg(struct module_env* env, struct query_info* qinfo,
	hashvalue_type hash, struct reply_info* rep, time_t leeway, int pside,
	struct reply_info* qrep, uint32_t flags, struct regional* region)
{
	struct msgreply_entry* e;
	time_t ttl = rep->ttl;
	size_t i;

	/* store RRsets */
        for(i=0; i<rep->rrset_count; i++) {
		rep->ref[i].key = rep->rrsets[i];
		rep->ref[i].id = rep->rrsets[i]->id;
	}

	/* there was a reply_info_sortref(rep) here but it seems to be
	 * unnecessary, because the cache gets locked per rrset. */
	reply_info_set_ttls(rep, *env->now);
	store_rrsets(env, rep, *env->now, leeway, pside, qrep, region);
	if(ttl == 0 && !(flags & DNSCACHE_STORE_ZEROTTL)) {
		/* we do not store the message, but we did store the RRs,
		 * which could be useful for delegation information */
		verbose(VERB_ALGO, "TTL 0: dropped msg from cache");
		free(rep);
		/* if the message is SERVFAIL in cache, remove that SERVFAIL,
		 * so that the TTL 0 response can be returned for future
		 * responses (i.e. don't get answered by the servfail from
		 * cache, but instead go to recursion to get this TTL0
		 * response). */
		msg_del_servfail(env, qinfo, flags);
		return;
	}

	/* store msg in the cache */
	reply_info_sortref(rep);
	if(!(e = query_info_entrysetup(qinfo, rep, hash))) {
		log_err("store_msg: malloc failed");
		return;
	}
	slabhash_insert(env->msg_cache, hash, &e->entry, rep, env->alloc);
}