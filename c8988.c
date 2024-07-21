generate_sub_request(uint8_t* qname, size_t qnamelen, uint16_t qtype, 
	uint16_t qclass, struct module_qstate* qstate, int id,
	struct iter_qstate* iq, enum iter_state initial_state, 
	enum iter_state finalstate, struct module_qstate** subq_ret, int v)
{
	struct module_qstate* subq = NULL;
	struct iter_qstate* subiq = NULL;
	uint16_t qflags = 0; /* OPCODE QUERY, no flags */
	struct query_info qinf;
	int prime = (finalstate == PRIME_RESP_STATE)?1:0;
	int valrec = 0;
	qinf.qname = qname;
	qinf.qname_len = qnamelen;
	qinf.qtype = qtype;
	qinf.qclass = qclass;
	qinf.local_alias = NULL;

	/* RD should be set only when sending the query back through the INIT
	 * state. */
	if(initial_state == INIT_REQUEST_STATE)
		qflags |= BIT_RD;
	/* We set the CD flag so we can send this through the "head" of 
	 * the resolution chain, which might have a validator. We are 
	 * uninterested in validating things not on the direct resolution 
	 * path.  */
	if(!v) {
		qflags |= BIT_CD;
		valrec = 1;
	}
	
	/* attach subquery, lookup existing or make a new one */
	fptr_ok(fptr_whitelist_modenv_attach_sub(qstate->env->attach_sub));
	if(!(*qstate->env->attach_sub)(qstate, &qinf, qflags, prime, valrec,
		&subq)) {
		return 0;
	}
	*subq_ret = subq;
	if(subq) {
		/* initialise the new subquery */
		subq->curmod = id;
		subq->ext_state[id] = module_state_initial;
		subq->minfo[id] = regional_alloc(subq->region, 
			sizeof(struct iter_qstate));
		if(!subq->minfo[id]) {
			log_err("init subq: out of memory");
			fptr_ok(fptr_whitelist_modenv_kill_sub(
				qstate->env->kill_sub));
			(*qstate->env->kill_sub)(subq);
			return 0;
		}
		subiq = (struct iter_qstate*)subq->minfo[id];
		memset(subiq, 0, sizeof(*subiq));
		subiq->num_target_queries = 0;
		target_count_create(iq);
		subiq->target_count = iq->target_count;
		if(iq->target_count)
			iq->target_count[0] ++; /* extra reference */
		subiq->num_current_queries = 0;
		subiq->depth = iq->depth+1;
		outbound_list_init(&subiq->outlist);
		subiq->state = initial_state;
		subiq->final_state = finalstate;
		subiq->qchase = subq->qinfo;
		subiq->chase_flags = subq->query_flags;
		subiq->refetch_glue = 0;
		if(qstate->env->cfg->qname_minimisation)
			subiq->minimisation_state = INIT_MINIMISE_STATE;
		else
			subiq->minimisation_state = DONOT_MINIMISE_STATE;
		memset(&subiq->qinfo_out, 0, sizeof(struct query_info));
	}
	return 1;
}