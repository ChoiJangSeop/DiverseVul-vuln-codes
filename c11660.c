mesh_state_create(struct module_env* env, struct query_info* qinfo,
	struct respip_client_info* cinfo, uint16_t qflags, int prime,
	int valrec)
{
	struct regional* region = alloc_reg_obtain(env->alloc);
	struct mesh_state* mstate;
	int i;
	if(!region)
		return NULL;
	mstate = (struct mesh_state*)regional_alloc(region, 
		sizeof(struct mesh_state));
	if(!mstate) {
		alloc_reg_release(env->alloc, region);
		return NULL;
	}
	memset(mstate, 0, sizeof(*mstate));
	mstate->node = *RBTREE_NULL;
	mstate->run_node = *RBTREE_NULL;
	mstate->node.key = mstate;
	mstate->run_node.key = mstate;
	mstate->reply_list = NULL;
	mstate->list_select = mesh_no_list;
	mstate->replies_sent = 0;
	rbtree_init(&mstate->super_set, &mesh_state_ref_compare);
	rbtree_init(&mstate->sub_set, &mesh_state_ref_compare);
	mstate->num_activated = 0;
	mstate->unique = NULL;
	/* init module qstate */
	mstate->s.qinfo.qtype = qinfo->qtype;
	mstate->s.qinfo.qclass = qinfo->qclass;
	mstate->s.qinfo.local_alias = NULL;
	mstate->s.qinfo.qname_len = qinfo->qname_len;
	mstate->s.qinfo.qname = regional_alloc_init(region, qinfo->qname,
		qinfo->qname_len);
	if(!mstate->s.qinfo.qname) {
		alloc_reg_release(env->alloc, region);
		return NULL;
	}
	if(cinfo) {
		mstate->s.client_info = regional_alloc_init(region, cinfo,
			sizeof(*cinfo));
		if(!mstate->s.client_info) {
			alloc_reg_release(env->alloc, region);
			return NULL;
		}
	}
	/* remove all weird bits from qflags */
	mstate->s.query_flags = (qflags & (BIT_RD|BIT_CD));
	mstate->s.is_priming = prime;
	mstate->s.is_valrec = valrec;
	mstate->s.reply = NULL;
	mstate->s.region = region;
	mstate->s.curmod = 0;
	mstate->s.return_msg = 0;
	mstate->s.return_rcode = LDNS_RCODE_NOERROR;
	mstate->s.env = env;
	mstate->s.mesh_info = mstate;
	mstate->s.prefetch_leeway = 0;
	mstate->s.serve_expired_data = NULL;
	mstate->s.no_cache_lookup = 0;
	mstate->s.no_cache_store = 0;
	mstate->s.need_refetch = 0;
	mstate->s.was_ratelimited = 0;

	/* init modules */
	for(i=0; i<env->mesh->mods.num; i++) {
		mstate->s.minfo[i] = NULL;
		mstate->s.ext_state[i] = module_state_initial;
	}
	/* init edns option lists */
	mstate->s.edns_opts_front_in = NULL;
	mstate->s.edns_opts_back_out = NULL;
	mstate->s.edns_opts_back_in = NULL;
	mstate->s.edns_opts_front_out = NULL;

	return mstate;
}