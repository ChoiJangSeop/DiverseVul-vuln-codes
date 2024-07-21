generate_dnskey_prefetch(struct module_qstate* qstate, 
	struct iter_qstate* iq, int id)
{
	struct module_qstate* subq;
	log_assert(iq->dp);

	/* is this query the same as the prefetch? */
	if(qstate->qinfo.qtype == LDNS_RR_TYPE_DNSKEY &&
		query_dname_compare(iq->dp->name, qstate->qinfo.qname)==0 &&
		(qstate->query_flags&BIT_RD) && !(qstate->query_flags&BIT_CD)){
		return;
	}

	/* if the DNSKEY is in the cache this lookup will stop quickly */
	log_nametypeclass(VERB_ALGO, "schedule dnskey prefetch", 
		iq->dp->name, LDNS_RR_TYPE_DNSKEY, iq->qchase.qclass);
	if(!generate_sub_request(iq->dp->name, iq->dp->namelen, 
		LDNS_RR_TYPE_DNSKEY, iq->qchase.qclass, qstate, id, iq,
		INIT_REQUEST_STATE, FINISHED_STATE, &subq, 0)) {
		/* we'll be slower, but it'll work */
		verbose(VERB_ALGO, "could not generate dnskey prefetch");
		return;
	}
	if(subq) {
		struct iter_qstate* subiq = 
			(struct iter_qstate*)subq->minfo[id];
		/* this qstate has the right delegation for the dnskey lookup*/
		/* make copy to avoid use of stub dp by different qs/threads */
		subiq->dp = delegpt_copy(iq->dp, subq->region);
		/* if !subiq->dp, it'll start from the cache, no problem */
	}
}