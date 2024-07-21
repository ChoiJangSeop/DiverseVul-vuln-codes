generate_target_query(struct module_qstate* qstate, struct iter_qstate* iq,
        int id, uint8_t* name, size_t namelen, uint16_t qtype, uint16_t qclass)
{
	struct module_qstate* subq;
	if(!generate_sub_request(name, namelen, qtype, qclass, qstate, 
		id, iq, INIT_REQUEST_STATE, FINISHED_STATE, &subq, 0))
		return 0;
	log_nametypeclass(VERB_QUERY, "new target", name, qtype, qclass);
	return 1;
}