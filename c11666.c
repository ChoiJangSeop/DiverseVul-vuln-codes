processFinished(struct module_qstate* qstate, struct iter_qstate* iq,
	int id)
{
	log_query_info(VERB_QUERY, "finishing processing for", 
		&qstate->qinfo);

	/* store negative cache element for parent side glue. */
	if(!qstate->no_cache_store && iq->query_for_pside_glue
		&& !iq->pside_glue)
			iter_store_parentside_neg(qstate->env, &qstate->qinfo,
				iq->deleg_msg?iq->deleg_msg->rep:
				(iq->response?iq->response->rep:NULL));
	if(!iq->response) {
		verbose(VERB_ALGO, "No response is set, servfail");
		errinf(qstate, "(no response found at query finish)");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}

	/* Make sure that the RA flag is set (since the presence of 
	 * this module means that recursion is available) */
	iq->response->rep->flags |= BIT_RA;

	/* Clear the AA flag */
	/* FIXME: does this action go here or in some other module? */
	iq->response->rep->flags &= ~BIT_AA;

	/* make sure QR flag is on */
	iq->response->rep->flags |= BIT_QR;

	/* we have finished processing this query */
	qstate->ext_state[id] = module_finished;

	/* TODO:  we are using a private TTL, trim the response. */
	/* if (mPrivateTTL > 0){IterUtils.setPrivateTTL(resp, mPrivateTTL); } */

	/* prepend any items we have accumulated */
	if(iq->an_prepend_list || iq->ns_prepend_list) {
		if(!iter_prepend(iq, iq->response, qstate->region)) {
			log_err("prepend rrsets: out of memory");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		/* reset the query name back */
		iq->response->qinfo = qstate->qinfo;
		/* the security state depends on the combination */
		iq->response->rep->security = sec_status_unchecked;
		/* store message with the finished prepended items,
		 * but only if we did recursion. The nonrecursion referral
		 * from cache does not need to be stored in the msg cache. */
		if(!qstate->no_cache_store && qstate->query_flags&BIT_RD) {
			iter_dns_store(qstate->env, &qstate->qinfo, 
				iq->response->rep, 0, qstate->prefetch_leeway,
				iq->dp&&iq->dp->has_parent_side_NS,
				qstate->region, qstate->query_flags);
		}
	}
	qstate->return_rcode = LDNS_RCODE_NOERROR;
	qstate->return_msg = iq->response;
	return 0;
}