dns64_inform_super(struct module_qstate* qstate, int id,
		struct module_qstate* super)
{
	struct dns64_qstate* super_dq = (struct dns64_qstate*)super->minfo[id];
	log_query_info(VERB_ALGO, "dns64: inform_super, sub is",
		       &qstate->qinfo);
	log_query_info(VERB_ALGO, "super is", &super->qinfo);

	/*
	 * Signal that the sub-query is finished, no matter whether we are
	 * successful or not. This lets the state machine terminate.
	 */
	if(!super_dq) {
		super_dq = (struct dns64_qstate*)regional_alloc(super->region,
			sizeof(*super_dq));
		if(!super_dq) {
			log_err("out of memory");
			super->return_rcode = LDNS_RCODE_SERVFAIL;
			super->return_msg = NULL;
			return;
		}
		super->minfo[id] = super_dq;
		memset(super_dq, 0, sizeof(*super_dq));
		super_dq->started_no_cache_store = super->no_cache_store;
	}
	super_dq->state = DNS64_SUBQUERY_FINISHED;

	/* If there is no successful answer, we're done. */
	if (qstate->return_rcode != LDNS_RCODE_NOERROR
	    || !qstate->return_msg
	    || !qstate->return_msg->rep) {
		return;
	}

	/* Use return code from A query in response to client. */
	if (super->return_rcode != LDNS_RCODE_NOERROR)
		super->return_rcode = qstate->return_rcode;

	/* Generate a response suitable for the original query. */
	if (qstate->qinfo.qtype == LDNS_RR_TYPE_A) {
		dns64_adjust_a(id, super, qstate);
	} else {
		log_assert(qstate->qinfo.qtype == LDNS_RR_TYPE_PTR);
		dns64_adjust_ptr(qstate, super);
	}

	/* Store the generated response in cache. */
	if ( (!super_dq || !super_dq->started_no_cache_store) &&
		!dns_cache_store(super->env, &super->qinfo, super->return_msg->rep,
		0, 0, 0, NULL, super->query_flags))
		log_err("out of memory");
}