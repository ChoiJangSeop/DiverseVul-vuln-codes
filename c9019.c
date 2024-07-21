processPrimeResponse(struct module_qstate* qstate, int id)
{
	struct iter_qstate* iq = (struct iter_qstate*)qstate->minfo[id];
	enum response_type type;
	iq->response->rep->flags &= ~(BIT_RD|BIT_RA); /* ignore rec-lame */
	type = response_type_from_server(
		(int)((iq->chase_flags&BIT_RD) || iq->chase_to_rd), 
		iq->response, &iq->qchase, iq->dp);
	if(type == RESPONSE_TYPE_ANSWER) {
		qstate->return_rcode = LDNS_RCODE_NOERROR;
		qstate->return_msg = iq->response;
	} else {
		errinf(qstate, "prime response did not get an answer");
		errinf_dname(qstate, "for", qstate->qinfo.qname);
		qstate->return_rcode = LDNS_RCODE_SERVFAIL;
		qstate->return_msg = NULL;
	}

	/* validate the root or stub after priming (if enabled).
	 * This is the same query as the prime query, but with validation.
	 * Now that we are primed, the additional queries that validation
	 * may need can be resolved, such as DLV. */
	if(qstate->env->cfg->harden_referral_path) {
		struct module_qstate* subq = NULL;
		log_nametypeclass(VERB_ALGO, "schedule prime validation", 
			qstate->qinfo.qname, qstate->qinfo.qtype,
			qstate->qinfo.qclass);
		if(!generate_sub_request(qstate->qinfo.qname, 
			qstate->qinfo.qname_len, qstate->qinfo.qtype,
			qstate->qinfo.qclass, qstate, id, iq,
			INIT_REQUEST_STATE, FINISHED_STATE, &subq, 1)) {
			verbose(VERB_ALGO, "could not generate prime check");
		}
		generate_a_aaaa_check(qstate, iq, id);
	}

	/* This event is finished. */
	qstate->ext_state[id] = module_finished;
	return 0;
}