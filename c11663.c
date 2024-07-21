cachedb_intcache_store(struct module_qstate* qstate)
{
	uint32_t store_flags = qstate->query_flags;

	if(qstate->env->cfg->serve_expired)
		store_flags |= DNSCACHE_STORE_ZEROTTL;
	if(!qstate->return_msg)
		return;
	(void)dns_cache_store(qstate->env, &qstate->qinfo,
		qstate->return_msg->rep, 0, qstate->prefetch_leeway, 0,
		qstate->region, store_flags);
}