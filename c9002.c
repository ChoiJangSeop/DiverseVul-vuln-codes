prime_stub(struct module_qstate* qstate, struct iter_qstate* iq, int id,
	uint8_t* qname, uint16_t qclass)
{
	/* Lookup the stub hint. This will return null if the stub doesn't 
	 * need to be re-primed. */
	struct iter_hints_stub* stub;
	struct delegpt* stub_dp;
	struct module_qstate* subq;

	if(!qname) return 0;
	stub = hints_lookup_stub(qstate->env->hints, qname, qclass, iq->dp);
	/* The stub (if there is one) does not need priming. */
	if(!stub)
		return 0;
	stub_dp = stub->dp;
	/* if we have an auth_zone dp, and stub is equal, don't prime stub
	 * yet, unless we want to fallback and avoid the auth_zone */
	if(!iq->auth_zone_avoid && iq->dp && iq->dp->auth_dp && 
		query_dname_compare(iq->dp->name, stub_dp->name) == 0)
		return 0;

	/* is it a noprime stub (always use) */
	if(stub->noprime) {
		int r = 0;
		if(iq->dp == NULL) r = 2;
		/* copy the dp out of the fixed hints structure, so that
		 * it can be changed when servicing this query */
		iq->dp = delegpt_copy(stub_dp, qstate->region);
		if(!iq->dp) {
			log_err("out of memory priming stub");
			errinf(qstate, "malloc failure, priming stub");
			(void)error_response(qstate, id, LDNS_RCODE_SERVFAIL);
			return 1; /* return 1 to make module stop, with error */
		}
		log_nametypeclass(VERB_DETAIL, "use stub", stub_dp->name, 
			LDNS_RR_TYPE_NS, qclass);
		return r;
	}

	/* Otherwise, we need to (re)prime the stub. */
	log_nametypeclass(VERB_DETAIL, "priming stub", stub_dp->name, 
		LDNS_RR_TYPE_NS, qclass);

	/* Stub priming events start at the QUERYTARGETS state to avoid the
	 * redundant INIT state processing. */
	if(!generate_sub_request(stub_dp->name, stub_dp->namelen, 
		LDNS_RR_TYPE_NS, qclass, qstate, id, iq,
		QUERYTARGETS_STATE, PRIME_RESP_STATE, &subq, 0)) {
		verbose(VERB_ALGO, "could not prime stub");
		errinf(qstate, "could not generate lookup for stub prime");
		(void)error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		return 1; /* return 1 to make module stop, with error */
	}
	if(subq) {
		struct iter_qstate* subiq = 
			(struct iter_qstate*)subq->minfo[id];

		/* Set the initial delegation point to the hint. */
		/* make copy to avoid use of stub dp by different qs/threads */
		subiq->dp = delegpt_copy(stub_dp, subq->region);
		if(!subiq->dp) {
			log_err("out of memory priming stub, copydp");
			fptr_ok(fptr_whitelist_modenv_kill_sub(
				qstate->env->kill_sub));
			(*qstate->env->kill_sub)(subq);
			errinf(qstate, "malloc failure, in stub prime");
			(void)error_response(qstate, id, LDNS_RCODE_SERVFAIL);
			return 1; /* return 1 to make module stop, with error */
		}
		/* there should not be any target queries -- although there 
		 * wouldn't be anyway, since stub hints never have 
		 * missing targets. */
		subiq->num_target_queries = 0; 
		subiq->wait_priming_stub = 1;
		subiq->dnssec_expected = iter_indicates_dnssec(
			qstate->env, subiq->dp, NULL, subq->qinfo.qclass);
	}
	
	/* this module stops, our submodule starts, and does the query. */
	qstate->ext_state[id] = module_wait_subquery;
	return 1;
}