processTargetResponse(struct module_qstate* qstate, int id,
	struct module_qstate* forq)
{
	struct iter_qstate* iq = (struct iter_qstate*)qstate->minfo[id];
	struct iter_qstate* foriq = (struct iter_qstate*)forq->minfo[id];
	struct ub_packed_rrset_key* rrset;
	struct delegpt_ns* dpns;
	log_assert(qstate->return_rcode == LDNS_RCODE_NOERROR);

	foriq->state = QUERYTARGETS_STATE;
	log_query_info(VERB_ALGO, "processTargetResponse", &qstate->qinfo);
	log_query_info(VERB_ALGO, "processTargetResponse super", &forq->qinfo);

	/* Tell the originating event that this target query has finished
	 * (regardless if it succeeded or not). */
	foriq->num_target_queries--;

	/* check to see if parent event is still interested (in orig name).  */
	if(!foriq->dp) {
		verbose(VERB_ALGO, "subq: parent not interested, was reset");
		return; /* not interested anymore */
	}
	dpns = delegpt_find_ns(foriq->dp, qstate->qinfo.qname,
			qstate->qinfo.qname_len);
	if(!dpns) {
		/* If not interested, just stop processing this event */
		verbose(VERB_ALGO, "subq: parent not interested anymore");
		/* could be because parent was jostled out of the cache,
		   and a new identical query arrived, that does not want it*/
		return;
	}

	/* if iq->query_for_pside_glue then add the pside_glue (marked lame) */
	if(iq->pside_glue) {
		/* if the pside_glue is NULL, then it could not be found,
		 * the done_pside is already set when created and a cache
		 * entry created in processFinished so nothing to do here */
		log_rrset_key(VERB_ALGO, "add parentside glue to dp", 
			iq->pside_glue);
		if(!delegpt_add_rrset(foriq->dp, forq->region, 
			iq->pside_glue, 1))
			log_err("out of memory adding pside glue");
	}

	/* This response is relevant to the current query, so we 
	 * add (attempt to add, anyway) this target(s) and reactivate 
	 * the original event. 
	 * NOTE: we could only look for the AnswerRRset if the 
	 * response type was ANSWER. */
	rrset = reply_find_answer_rrset(&iq->qchase, qstate->return_msg->rep);
	if(rrset) {
		/* if CNAMEs have been followed - add new NS to delegpt. */
		/* BTW. RFC 1918 says NS should not have got CNAMEs. Robust. */
		if(!delegpt_find_ns(foriq->dp, rrset->rk.dname, 
			rrset->rk.dname_len)) {
			/* if dpns->lame then set newcname ns lame too */
			if(!delegpt_add_ns(foriq->dp, forq->region, 
				rrset->rk.dname, dpns->lame))
				log_err("out of memory adding cnamed-ns");
		}
		/* if dpns->lame then set the address(es) lame too */
		if(!delegpt_add_rrset(foriq->dp, forq->region, rrset, 
			dpns->lame))
			log_err("out of memory adding targets");
		verbose(VERB_ALGO, "added target response");
		delegpt_log(VERB_ALGO, foriq->dp);
	} else {
		verbose(VERB_ALGO, "iterator TargetResponse failed");
		dpns->resolved = 1; /* fail the target */
	}
}