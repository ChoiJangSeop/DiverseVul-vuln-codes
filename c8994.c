generate_a_aaaa_check(struct module_qstate* qstate, struct iter_qstate* iq, 
	int id)
{
	struct iter_env* ie = (struct iter_env*)qstate->env->modinfo[id];
	struct module_qstate* subq;
	size_t i;
	struct reply_info* rep = iq->response->rep;
	struct ub_packed_rrset_key* s;
	log_assert(iq->dp);

	if(iq->depth == ie->max_dependency_depth)
		return;
	/* walk through additional, and check if in-zone,
	 * only relevant A, AAAA are left after scrub anyway */
	for(i=rep->an_numrrsets+rep->ns_numrrsets; i<rep->rrset_count; i++) {
		s = rep->rrsets[i];
		/* check *ALL* addresses that are transmitted in additional*/
		/* is it an address ? */
		if( !(ntohs(s->rk.type)==LDNS_RR_TYPE_A ||
			ntohs(s->rk.type)==LDNS_RR_TYPE_AAAA)) {
			continue;
		}
		/* is this query the same as the A/AAAA check for it */
		if(qstate->qinfo.qtype == ntohs(s->rk.type) &&
			qstate->qinfo.qclass == ntohs(s->rk.rrset_class) &&
			query_dname_compare(qstate->qinfo.qname, 
				s->rk.dname)==0 &&
			(qstate->query_flags&BIT_RD) && 
			!(qstate->query_flags&BIT_CD))
			continue;

		/* generate subrequest for it */
		log_nametypeclass(VERB_ALGO, "schedule addr fetch", 
			s->rk.dname, ntohs(s->rk.type), 
			ntohs(s->rk.rrset_class));
		if(!generate_sub_request(s->rk.dname, s->rk.dname_len, 
			ntohs(s->rk.type), ntohs(s->rk.rrset_class),
			qstate, id, iq,
			INIT_REQUEST_STATE, FINISHED_STATE, &subq, 1)) {
			verbose(VERB_ALGO, "could not generate addr check");
			return;
		}
		/* ignore subq - not need for more init */
	}
}