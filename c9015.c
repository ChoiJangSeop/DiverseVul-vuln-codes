iter_new(struct module_qstate* qstate, int id)
{
	struct iter_qstate* iq = (struct iter_qstate*)regional_alloc(
		qstate->region, sizeof(struct iter_qstate));
	qstate->minfo[id] = iq;
	if(!iq) 
		return 0;
	memset(iq, 0, sizeof(*iq));
	iq->state = INIT_REQUEST_STATE;
	iq->final_state = FINISHED_STATE;
	iq->an_prepend_list = NULL;
	iq->an_prepend_last = NULL;
	iq->ns_prepend_list = NULL;
	iq->ns_prepend_last = NULL;
	iq->dp = NULL;
	iq->depth = 0;
	iq->num_target_queries = 0;
	iq->num_current_queries = 0;
	iq->query_restart_count = 0;
	iq->referral_count = 0;
	iq->sent_count = 0;
	iq->ratelimit_ok = 0;
	iq->target_count = NULL;
	iq->wait_priming_stub = 0;
	iq->refetch_glue = 0;
	iq->dnssec_expected = 0;
	iq->dnssec_lame_query = 0;
	iq->chase_flags = qstate->query_flags;
	/* Start with the (current) qname. */
	iq->qchase = qstate->qinfo;
	outbound_list_init(&iq->outlist);
	iq->minimise_count = 0;
	iq->timeout_count = 0;
	if (qstate->env->cfg->qname_minimisation)
		iq->minimisation_state = INIT_MINIMISE_STATE;
	else
		iq->minimisation_state = DONOT_MINIMISE_STATE;
	
	memset(&iq->qinfo_out, 0, sizeof(struct query_info));
	return 1;
}