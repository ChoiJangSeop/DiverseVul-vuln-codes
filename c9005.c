processDSNSFind(struct module_qstate* qstate, struct iter_qstate* iq, int id)
{
	struct module_qstate* subq = NULL;
	verbose(VERB_ALGO, "processDSNSFind");

	if(!iq->dsns_point) {
		/* initialize */
		iq->dsns_point = iq->qchase.qname;
		iq->dsns_point_len = iq->qchase.qname_len;
	}
	/* robustcheck for internal error: we are not underneath the dp */
	if(!dname_subdomain_c(iq->dsns_point, iq->dp->name)) {
		errinf_dname(qstate, "for DS query parent-child nameserver search the query is not under the zone", iq->dp->name);
		return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
	}

	/* go up one (more) step, until we hit the dp, if so, end */
	dname_remove_label(&iq->dsns_point, &iq->dsns_point_len);
	if(query_dname_compare(iq->dsns_point, iq->dp->name) == 0) {
		/* there was no inbetween nameserver, use the old delegation
		 * point again.  And this time, because dsns_point is nonNULL
		 * we are going to accept the (bad) result */
		iq->state = QUERYTARGETS_STATE;
		return 1;
	}
	iq->state = DSNS_FIND_STATE;

	/* spawn NS lookup (validation not needed, this is for DS lookup) */
	log_nametypeclass(VERB_ALGO, "fetch nameservers", 
		iq->dsns_point, LDNS_RR_TYPE_NS, iq->qchase.qclass);
	if(!generate_sub_request(iq->dsns_point, iq->dsns_point_len, 
		LDNS_RR_TYPE_NS, iq->qchase.qclass, qstate, id, iq,
		INIT_REQUEST_STATE, FINISHED_STATE, &subq, 0)) {
		errinf_dname(qstate, "for DS query parent-child nameserver search, could not generate NS lookup for", iq->dsns_point);
		return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
	}

	return 0;
}