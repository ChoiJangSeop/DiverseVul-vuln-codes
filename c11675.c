error_response_cache(struct module_qstate* qstate, int id, int rcode)
{
	if(!qstate->no_cache_store) {
		/* store in cache */
		struct reply_info err;
		if(qstate->prefetch_leeway > NORR_TTL) {
			verbose(VERB_ALGO, "error response for prefetch in cache");
			/* attempt to adjust the cache entry prefetch */
			if(dns_cache_prefetch_adjust(qstate->env, &qstate->qinfo,
				NORR_TTL, qstate->query_flags))
				return error_response(qstate, id, rcode);
			/* if that fails (not in cache), fall through to store err */
		}
		if(qstate->env->cfg->serve_expired) {
			/* if serving expired contents, and such content is
			 * already available, don't overwrite this servfail */
			struct msgreply_entry* msg;
			if((msg=msg_cache_lookup(qstate->env,
				qstate->qinfo.qname, qstate->qinfo.qname_len,
				qstate->qinfo.qtype, qstate->qinfo.qclass,
				qstate->query_flags, 0,
				qstate->env->cfg->serve_expired_ttl_reset))
				!= NULL) {
				if(qstate->env->cfg->serve_expired_ttl_reset) {
					struct reply_info* rep =
						(struct reply_info*)msg->entry.data;
					if(rep && *qstate->env->now +
						qstate->env->cfg->serve_expired_ttl  >
						rep->serve_expired_ttl) {
						rep->serve_expired_ttl =
							*qstate->env->now +
							qstate->env->cfg->serve_expired_ttl;
					}
				}
				lock_rw_unlock(&msg->entry.lock);
				return error_response(qstate, id, rcode);
			}
			/* serving expired contents, but nothing is cached
			 * at all, so the servfail cache entry is useful
			 * (stops waste of time on this servfail NORR_TTL) */
		} else {
			/* don't overwrite existing (non-expired) data in
			 * cache with a servfail */
			struct msgreply_entry* msg;
			if((msg=msg_cache_lookup(qstate->env,
				qstate->qinfo.qname, qstate->qinfo.qname_len,
				qstate->qinfo.qtype, qstate->qinfo.qclass,
				qstate->query_flags, *qstate->env->now, 0))
				!= NULL) {
				struct reply_info* rep = (struct reply_info*)
					msg->entry.data;
				if(FLAGS_GET_RCODE(rep->flags) ==
					LDNS_RCODE_NOERROR ||
					FLAGS_GET_RCODE(rep->flags) ==
					LDNS_RCODE_NXDOMAIN) {
					/* we have a good entry,
					 * don't overwrite */
					lock_rw_unlock(&msg->entry.lock);
					return error_response(qstate, id, rcode);
				}
				lock_rw_unlock(&msg->entry.lock);
			}
			
		}
		memset(&err, 0, sizeof(err));
		err.flags = (uint16_t)(BIT_QR | BIT_RA);
		FLAGS_SET_RCODE(err.flags, rcode);
		err.qdcount = 1;
		err.ttl = NORR_TTL;
		err.prefetch_ttl = PREFETCH_TTL_CALC(err.ttl);
		err.serve_expired_ttl = NORR_TTL;
		/* do not waste time trying to validate this servfail */
		err.security = sec_status_indeterminate;
		verbose(VERB_ALGO, "store error response in message cache");
		iter_dns_store(qstate->env, &qstate->qinfo, &err, 0, 0, 0, NULL,
			qstate->query_flags);
	}
	return error_response(qstate, id, rcode);
}