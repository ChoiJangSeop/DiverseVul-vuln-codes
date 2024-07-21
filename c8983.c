error_supers(struct module_qstate* qstate, int id, struct module_qstate* super)
{
	struct iter_qstate* super_iq = (struct iter_qstate*)super->minfo[id];

	if(qstate->qinfo.qtype == LDNS_RR_TYPE_A ||
		qstate->qinfo.qtype == LDNS_RR_TYPE_AAAA) {
		/* mark address as failed. */
		struct delegpt_ns* dpns = NULL;
		super_iq->num_target_queries--; 
		if(super_iq->dp)
			dpns = delegpt_find_ns(super_iq->dp, 
				qstate->qinfo.qname, qstate->qinfo.qname_len);
		if(!dpns) {
			/* not interested */
			/* this can happen, for eg. qname minimisation asked
			 * for an NXDOMAIN to be validated, and used qtype
			 * A for that, and the error of that, the name, is
			 * not listed in super_iq->dp */
			verbose(VERB_ALGO, "subq error, but not interested");
			log_query_info(VERB_ALGO, "superq", &super->qinfo);
			return;
		} else {
			/* see if the failure did get (parent-lame) info */
			if(!cache_fill_missing(super->env, super_iq->qchase.qclass,
				super->region, super_iq->dp))
				log_err("out of memory adding missing");
		}
		dpns->resolved = 1; /* mark as failed */
	}
	if(qstate->qinfo.qtype == LDNS_RR_TYPE_NS) {
		/* prime failed to get delegation */
		super_iq->dp = NULL;
	}
	/* evaluate targets again */
	super_iq->state = QUERYTARGETS_STATE; 
	/* super becomes runnable, and will process this change */
}