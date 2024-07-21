answer_norec_from_cache(struct worker* worker, struct query_info* qinfo,
	uint16_t id, uint16_t flags, struct comm_reply* repinfo, 
	struct edns_data* edns)
{
	/* for a nonrecursive query return either:
	 * 	o an error (servfail; we try to avoid this)
	 * 	o a delegation (closest we have; this routine tries that)
	 * 	o the answer (checked by answer_from_cache) 
	 *
	 * So, grab a delegation from the rrset cache. 
	 * Then check if it needs validation, if so, this routine fails,
	 * so that iterator can prime and validator can verify rrsets.
	 */
	uint16_t udpsize = edns->udp_size;
	int secure = 0;
	time_t timenow = *worker->env.now;
	int must_validate = (!(flags&BIT_CD) || worker->env.cfg->ignore_cd)
		&& worker->env.need_to_validate;
	struct dns_msg *msg = NULL;
	struct delegpt *dp;

	dp = dns_cache_find_delegation(&worker->env, qinfo->qname, 
		qinfo->qname_len, qinfo->qtype, qinfo->qclass,
		worker->scratchpad, &msg, timenow);
	if(!dp) { /* no delegation, need to reprime */
		return 0;
	}
	/* In case we have a local alias, copy it into the delegation message.
	 * Shallow copy should be fine, as we'll be done with msg in this
	 * function. */
	msg->qinfo.local_alias = qinfo->local_alias;
	if(must_validate) {
		switch(check_delegation_secure(msg->rep)) {
		case sec_status_unchecked:
			/* some rrsets have not been verified yet, go and 
			 * let validator do that */
			return 0;
		case sec_status_bogus:
		case sec_status_secure_sentinel_fail:
			/* some rrsets are bogus, reply servfail */
			edns->edns_version = EDNS_ADVERTISED_VERSION;
			edns->udp_size = EDNS_ADVERTISED_SIZE;
			edns->ext_rcode = 0;
			edns->bits &= EDNS_DO;
			if(!inplace_cb_reply_servfail_call(&worker->env, qinfo, NULL,
				msg->rep, LDNS_RCODE_SERVFAIL, edns, repinfo, worker->scratchpad,
				worker->env.now_tv))
					return 0;
			/* TODO store the reason for the bogus reply in cache
			 * and implement in here instead of the hardcoded EDE */
			if (worker->env.cfg->ede) {
				EDNS_OPT_LIST_APPEND_EDE(&edns->opt_list_out,
					worker->scratchpad, LDNS_EDE_DNSSEC_BOGUS, "");
			}
			error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
				&msg->qinfo, id, flags, edns);
			if(worker->stats.extended) {
				worker->stats.ans_bogus++;
				worker->stats.ans_rcode[LDNS_RCODE_SERVFAIL]++;
			}
			return 1;
		case sec_status_secure:
			/* all rrsets are secure */
			/* remove non-secure rrsets from the add. section*/
			if(worker->env.cfg->val_clean_additional)
				deleg_remove_nonsecure_additional(msg->rep);
			secure = 1;
			break;
		case sec_status_indeterminate:
		case sec_status_insecure:
		default:
			/* not secure */
			secure = 0;
			break;
		}
	}
	/* return this delegation from the cache */
	edns->edns_version = EDNS_ADVERTISED_VERSION;
	edns->udp_size = EDNS_ADVERTISED_SIZE;
	edns->ext_rcode = 0;
	edns->bits &= EDNS_DO;
	if(!inplace_cb_reply_cache_call(&worker->env, qinfo, NULL, msg->rep,
		(int)(flags&LDNS_RCODE_MASK), edns, repinfo, worker->scratchpad,
		worker->env.now_tv))
			return 0;
	msg->rep->flags |= BIT_QR|BIT_RA;
	if(!reply_info_answer_encode(&msg->qinfo, msg->rep, id, flags,
		repinfo->c->buffer, 0, 1, worker->scratchpad,
		udpsize, edns, (int)(edns->bits & EDNS_DO), secure)) {
		if(!inplace_cb_reply_servfail_call(&worker->env, qinfo, NULL, NULL,
			LDNS_RCODE_SERVFAIL, edns, repinfo, worker->scratchpad,
			worker->env.now_tv))
				edns->opt_list_inplace_cb_out = NULL;
		error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
			&msg->qinfo, id, flags, edns);
	}
	if(worker->stats.extended) {
		if(secure) worker->stats.ans_secure++;
		server_stats_insrcode(&worker->stats, repinfo->c->buffer);
	}
	return 1;
}