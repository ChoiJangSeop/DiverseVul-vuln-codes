prime_root(struct module_qstate* qstate, struct iter_qstate* iq, int id,
	uint16_t qclass)
{
	struct delegpt* dp;
	struct module_qstate* subq;
	verbose(VERB_DETAIL, "priming . %s NS", 
		sldns_lookup_by_id(sldns_rr_classes, (int)qclass)?
		sldns_lookup_by_id(sldns_rr_classes, (int)qclass)->name:"??");
	dp = hints_lookup_root(qstate->env->hints, qclass);
	if(!dp) {
		verbose(VERB_ALGO, "Cannot prime due to lack of hints");
		return 0;
	}
	/* Priming requests start at the QUERYTARGETS state, skipping 
	 * the normal INIT state logic (which would cause an infloop). */
	if(!generate_sub_request((uint8_t*)"\000", 1, LDNS_RR_TYPE_NS, 
		qclass, qstate, id, iq, QUERYTARGETS_STATE, PRIME_RESP_STATE,
		&subq, 0)) {
		verbose(VERB_ALGO, "could not prime root");
		return 0;
	}
	if(subq) {
		struct iter_qstate* subiq = 
			(struct iter_qstate*)subq->minfo[id];
		/* Set the initial delegation point to the hint.
		 * copy dp, it is now part of the root prime query. 
		 * dp was part of in the fixed hints structure. */
		subiq->dp = delegpt_copy(dp, subq->region);
		if(!subiq->dp) {
			log_err("out of memory priming root, copydp");
			fptr_ok(fptr_whitelist_modenv_kill_sub(
				qstate->env->kill_sub));
			(*qstate->env->kill_sub)(subq);
			return 0;
		}
		/* there should not be any target queries. */
		subiq->num_target_queries = 0; 
		subiq->dnssec_expected = iter_indicates_dnssec(
			qstate->env, subiq->dp, NULL, subq->qinfo.qclass);
	}
	
	/* this module stops, our submodule starts, and does the query. */
	qstate->ext_state[id] = module_wait_subquery;
	return 1;
}