store_rrsets(struct module_env* env, struct reply_info* rep, time_t now,
	time_t leeway, int pside, struct reply_info* qrep,
	struct regional* region)
{
	size_t i;
	/* see if rrset already exists in cache, if not insert it. */
	for(i=0; i<rep->rrset_count; i++) {
		rep->ref[i].key = rep->rrsets[i];
		rep->ref[i].id = rep->rrsets[i]->id;
		/* update ref if it was in the cache */
		switch(rrset_cache_update(env->rrset_cache, &rep->ref[i],
				env->alloc, now + ((ntohs(rep->ref[i].key->rk.type)==
				LDNS_RR_TYPE_NS && !pside)?0:leeway))) {
		case 0: /* ref unchanged, item inserted */
			break;
		case 2: /* ref updated, cache is superior */
			if(region) {
				struct ub_packed_rrset_key* ck;
				lock_rw_rdlock(&rep->ref[i].key->entry.lock);
				/* if deleted rrset, do not copy it */
				if(rep->ref[i].key->id == 0)
					ck = NULL;
				else 	ck = packed_rrset_copy_region(
					rep->ref[i].key, region, now);
				lock_rw_unlock(&rep->ref[i].key->entry.lock);
				if(ck) {
					/* use cached copy if memory allows */
					qrep->rrsets[i] = ck;
				}
			}
			/* no break: also copy key item */
			/* the line below is matched by gcc regex and silences
			 * the fallthrough warning */
			/* fallthrough */
		case 1: /* ref updated, item inserted */
			rep->rrsets[i] = rep->ref[i].key;
		}
	}
}