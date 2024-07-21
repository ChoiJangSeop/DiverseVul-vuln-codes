handle_event_moddone(struct module_qstate* qstate, int id)
{
	struct dns64_qstate* iq = (struct dns64_qstate*)qstate->minfo[id];
    /*
     * In many cases we have nothing special to do. From most to least common:
     *
     *   - An internal query.
     *   - A query for a record type other than AAAA.
     *   - CD FLAG was set on querier
     *   - An AAAA query for which an error was returned.(qstate.return_rcode)
     *     -> treated as servfail thus synthesize (sec 5.1.3 6147), thus
     *        synthesize in (sec 5.1.2 of RFC6147).
     *   - A successful AAAA query with an answer.
     */
	if((!iq || iq->state != DNS64_INTERNAL_QUERY)
            && qstate->qinfo.qtype == LDNS_RR_TYPE_AAAA
	    && !(qstate->query_flags & BIT_CD)
	    && !(qstate->return_msg &&
		    qstate->return_msg->rep &&
		    reply_find_answer_rrset(&qstate->qinfo,
			    qstate->return_msg->rep)))
		/* not internal, type AAAA, not CD, and no answer RRset,
		 * So, this is a AAAA noerror/nodata answer */
		return generate_type_A_query(qstate, id);

	if((!iq || iq->state != DNS64_INTERNAL_QUERY)
	    && qstate->qinfo.qtype == LDNS_RR_TYPE_AAAA
	    && !(qstate->query_flags & BIT_CD)
	    && dns64_always_synth_for_qname(qstate, id)) {
		/* if it is not internal, AAAA, not CD and listed domain,
		 * generate from A record and ignore AAAA */
		verbose(VERB_ALGO, "dns64: ignore-aaaa and synthesize anyway");
		return generate_type_A_query(qstate, id);
	}

	/* Store the response in cache. */
	if ( (!iq || !iq->started_no_cache_store) &&
		qstate->return_msg && qstate->return_msg->rep &&
		!dns_cache_store(qstate->env, &qstate->qinfo, qstate->return_msg->rep,
		0, 0, 0, NULL, qstate->query_flags))
		log_err("out of memory");

	/* do nothing */
	return module_finished;
}