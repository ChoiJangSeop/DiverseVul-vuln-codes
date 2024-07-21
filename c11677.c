dns_cache_store(struct module_env* env, struct query_info* msgqinf,
        struct reply_info* msgrep, int is_referral, time_t leeway, int pside,
	struct regional* region, uint32_t flags)
{
	struct reply_info* rep = NULL;
	/* alloc, malloc properly (not in region, like msg is) */
	rep = reply_info_copy(msgrep, env->alloc, NULL);
	if(!rep)
		return 0;
	/* ttl must be relative ;i.e. 0..86400 not  time(0)+86400.
	 * the env->now is added to message and RRsets in this routine. */
	/* the leeway is used to invalidate other rrsets earlier */

	if(is_referral) {
		/* store rrsets */
		struct rrset_ref ref;
		size_t i;
		for(i=0; i<rep->rrset_count; i++) {
			packed_rrset_ttl_add((struct packed_rrset_data*)
				rep->rrsets[i]->entry.data, *env->now);
			ref.key = rep->rrsets[i];
			ref.id = rep->rrsets[i]->id;
			/*ignore ret: it was in the cache, ref updated */
			/* no leeway for typeNS */
			(void)rrset_cache_update(env->rrset_cache, &ref, 
				env->alloc, *env->now + 
				((ntohs(ref.key->rk.type)==LDNS_RR_TYPE_NS
				 && !pside) ? 0:leeway));
		}
		free(rep);
		return 1;
	} else {
		/* store msg, and rrsets */
		struct query_info qinf;
		hashvalue_type h;

		qinf = *msgqinf;
		qinf.qname = memdup(msgqinf->qname, msgqinf->qname_len);
		if(!qinf.qname) {
			reply_info_parsedelete(rep, env->alloc);
			return 0;
		}
		/* fixup flags to be sensible for a reply based on the cache */
		/* this module means that RA is available. It is an answer QR. 
		 * Not AA from cache. Not CD in cache (depends on client bit). */
		rep->flags |= (BIT_RA | BIT_QR);
		rep->flags &= ~(BIT_AA | BIT_CD);
		h = query_info_hash(&qinf, (uint16_t)flags);
		dns_cache_store_msg(env, &qinf, h, rep, leeway, pside, msgrep,
			flags, region);
		/* qname is used inside query_info_entrysetup, and set to 
		 * NULL. If it has not been used, free it. free(0) is safe. */
		free(qinf.qname);
	}
	return 1;
}