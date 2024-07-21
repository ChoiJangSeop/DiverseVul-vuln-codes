processCollectClass(struct module_qstate* qstate, int id)
{
	struct iter_qstate* iq = (struct iter_qstate*)qstate->minfo[id];
	struct module_qstate* subq;
	/* If qchase.qclass == 0 then send out queries for all classes.
	 * Otherwise, do nothing (wait for all answers to arrive and the
	 * processClassResponse to put them together, and that moves us
	 * towards the Finished state when done. */
	if(iq->qchase.qclass == 0) {
		uint16_t c = 0;
		iq->qchase.qclass = LDNS_RR_CLASS_ANY;
		while(iter_get_next_root(qstate->env->hints,
			qstate->env->fwds, &c)) {
			/* generate query for this class */
			log_nametypeclass(VERB_ALGO, "spawn collect query",
				qstate->qinfo.qname, qstate->qinfo.qtype, c);
			if(!generate_sub_request(qstate->qinfo.qname,
				qstate->qinfo.qname_len, qstate->qinfo.qtype,
				c, qstate, id, iq, INIT_REQUEST_STATE,
				FINISHED_STATE, &subq, 
				(int)!(qstate->query_flags&BIT_CD))) {
				errinf(qstate, "could not generate class ANY"
					" lookup query");
				return error_response(qstate, id, 
					LDNS_RCODE_SERVFAIL);
			}
			/* ignore subq, no special init required */
			iq->num_current_queries ++;
			if(c == 0xffff)
				break;
			else c++;
		}
		/* if no roots are configured at all, return */
		if(iq->num_current_queries == 0) {
			verbose(VERB_ALGO, "No root hints or fwds, giving up "
				"on qclass ANY");
			return error_response(qstate, id, LDNS_RCODE_REFUSED);
		}
		/* return false, wait for queries to return */
	}
	/* if woke up here because of an answer, wait for more answers */
	return 0;
}