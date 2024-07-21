generate_ns_check(struct module_qstate* qstate, struct iter_qstate* iq, int id)
{
	struct iter_env* ie = (struct iter_env*)qstate->env->modinfo[id];
	struct module_qstate* subq;
	log_assert(iq->dp);

	if(iq->depth == ie->max_dependency_depth)
		return;
	if(!can_have_last_resort(qstate->env, iq->dp->name, iq->dp->namelen,
		iq->qchase.qclass, NULL))
		return;
	/* is this query the same as the nscheck? */
	if(qstate->qinfo.qtype == LDNS_RR_TYPE_NS &&
		query_dname_compare(iq->dp->name, qstate->qinfo.qname)==0 &&
		(qstate->query_flags&BIT_RD) && !(qstate->query_flags&BIT_CD)){
		/* spawn off A, AAAA queries for in-zone glue to check */
		generate_a_aaaa_check(qstate, iq, id);
		return;
	}
	/* no need to get the NS record for DS, it is above the zonecut */
	if(qstate->qinfo.qtype == LDNS_RR_TYPE_DS)
		return;

	log_nametypeclass(VERB_ALGO, "schedule ns fetch", 
		iq->dp->name, LDNS_RR_TYPE_NS, iq->qchase.qclass);
	if(!generate_sub_request(iq->dp->name, iq->dp->namelen, 
		LDNS_RR_TYPE_NS, iq->qchase.qclass, qstate, id, iq,
		INIT_REQUEST_STATE, FINISHED_STATE, &subq, 1)) {
		verbose(VERB_ALGO, "could not generate ns check");
		return;
	}
	if(subq) {
		struct iter_qstate* subiq = 
			(struct iter_qstate*)subq->minfo[id];

		/* make copy to avoid use of stub dp by different qs/threads */
		/* refetch glue to start higher up the tree */
		subiq->refetch_glue = 1;
		subiq->dp = delegpt_copy(iq->dp, subq->region);
		if(!subiq->dp) {
			log_err("out of memory generating ns check, copydp");
			fptr_ok(fptr_whitelist_modenv_kill_sub(
				qstate->env->kill_sub));
			(*qstate->env->kill_sub)(subq);
			return;
		}
	}
}