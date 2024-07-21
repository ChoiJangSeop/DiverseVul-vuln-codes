int storeQueryInCache(struct module_qstate* qstate, struct query_info* qinfo,
	struct reply_info* msgrep, int is_referral)
{
	if (!msgrep)
		return 0;

	/* authoritative answer can't be stored in cache */
	if (msgrep->authoritative) {
		PyErr_SetString(PyExc_ValueError,
			"Authoritative answer can't be stored");
		return 0;
	}

	return dns_cache_store(qstate->env, qinfo, msgrep, is_referral,
		qstate->prefetch_leeway, 0, NULL, qstate->query_flags);
}