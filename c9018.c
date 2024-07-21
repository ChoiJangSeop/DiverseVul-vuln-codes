generate_parentside_target_query(struct module_qstate* qstate, 
	struct iter_qstate* iq, int id, uint8_t* name, size_t namelen, 
	uint16_t qtype, uint16_t qclass)
{
	struct module_qstate* subq;
	if(!generate_sub_request(name, namelen, qtype, qclass, qstate, 
		id, iq, INIT_REQUEST_STATE, FINISHED_STATE, &subq, 0))
		return 0;
	if(subq) {
		struct iter_qstate* subiq = 
			(struct iter_qstate*)subq->minfo[id];
		/* blacklist the cache - we want to fetch parent stuff */
		sock_list_insert(&subq->blacklist, NULL, 0, subq->region);
		subiq->query_for_pside_glue = 1;
		if(dname_subdomain_c(name, iq->dp->name)) {
			subiq->dp = delegpt_copy(iq->dp, subq->region);
			subiq->dnssec_expected = iter_indicates_dnssec(
				qstate->env, subiq->dp, NULL, 
				subq->qinfo.qclass);
			subiq->refetch_glue = 1;
		} else {
			subiq->dp = dns_cache_find_delegation(qstate->env, 
				name, namelen, qtype, qclass, subq->region,
				&subiq->deleg_msg,
				*qstate->env->now+subq->prefetch_leeway); 
			/* if no dp, then it's from root, refetch unneeded */
			if(subiq->dp) { 
				subiq->dnssec_expected = iter_indicates_dnssec(
					qstate->env, subiq->dp, NULL, 
					subq->qinfo.qclass);
				subiq->refetch_glue = 1;
			}
		}
	}
	log_nametypeclass(VERB_QUERY, "new pside target", name, qtype, qclass);
	return 1;
}