processFinished(struct module_qstate* qstate, struct val_qstate* vq, 
	struct val_env* ve, int id)
{
	enum val_classification subtype = val_classify_response(
		qstate->query_flags, &qstate->qinfo, &vq->qchase, 
		vq->orig_msg->rep, vq->rrset_skip);

	/* store overall validation result in orig_msg */
	if(vq->rrset_skip == 0) {
		vq->orig_msg->rep->security = vq->chase_reply->security;
		update_reason_bogus(vq->orig_msg->rep, vq->chase_reply->reason_bogus);
	} else if(subtype != VAL_CLASS_REFERRAL ||
		vq->rrset_skip < vq->orig_msg->rep->an_numrrsets + 
		vq->orig_msg->rep->ns_numrrsets) {
		/* ignore sec status of additional section if a referral 
		 * type message skips there and
		 * use the lowest security status as end result. */
		if(vq->chase_reply->security < vq->orig_msg->rep->security) {
			vq->orig_msg->rep->security = 
				vq->chase_reply->security;
			update_reason_bogus(vq->orig_msg->rep, vq->chase_reply->reason_bogus);
		}
	}

	if(subtype == VAL_CLASS_REFERRAL) {
		/* for a referral, move to next unchecked rrset and check it*/
		vq->rrset_skip = val_next_unchecked(vq->orig_msg->rep, 
			vq->rrset_skip);
		if(vq->rrset_skip < vq->orig_msg->rep->rrset_count) {
			/* and restart for this rrset */
			verbose(VERB_ALGO, "validator: go to next rrset");
			vq->chase_reply->security = sec_status_unchecked;
			vq->state = VAL_INIT_STATE;
			return 1;
		}
		/* referral chase is done */
	}
	if(vq->chase_reply->security != sec_status_bogus &&
		subtype == VAL_CLASS_CNAME) {
		/* chase the CNAME; process next part of the message */
		if(!val_chase_cname(&vq->qchase, vq->orig_msg->rep, 
			&vq->rrset_skip)) {
			verbose(VERB_ALGO, "validator: failed to chase CNAME");
			vq->orig_msg->rep->security = sec_status_bogus;
			update_reason_bogus(vq->orig_msg->rep, LDNS_EDE_DNSSEC_BOGUS);
		} else {
			/* restart process for new qchase at rrset_skip */
			log_query_info(VERB_ALGO, "validator: chased to",
				&vq->qchase);
			vq->chase_reply->security = sec_status_unchecked;
			vq->state = VAL_INIT_STATE;
			return 1;
		}
	}

	if(vq->orig_msg->rep->security == sec_status_secure) {
		/* If the message is secure, check that all rrsets are
		 * secure (i.e. some inserted RRset for CNAME chain with
		 * a different signer name). And drop additional rrsets
		 * that are not secure (if clean-additional option is set) */
		/* this may cause the msg to be marked bogus */
		val_check_nonsecure(qstate->env, vq->orig_msg->rep);
		if(vq->orig_msg->rep->security == sec_status_secure) {
			log_query_info(VERB_DETAIL, "validation success", 
				&qstate->qinfo);
			if(!qstate->no_cache_store) {
				val_neg_addreply(qstate->env->neg_cache,
					vq->orig_msg->rep);
			}
		}
	}

	/* if the result is bogus - set message ttl to bogus ttl to avoid
	 * endless bogus revalidation */
	if(vq->orig_msg->rep->security == sec_status_bogus) {
		/* see if we can try again to fetch data */
		if(vq->restart_count < ve->max_restart) {
			int restart_count = vq->restart_count+1;
			verbose(VERB_ALGO, "validation failed, "
				"blacklist and retry to fetch data");
			val_blacklist(&qstate->blacklist, qstate->region, 
				qstate->reply_origin, 0);
			qstate->reply_origin = NULL;
			qstate->errinf = NULL;
			memset(vq, 0, sizeof(*vq));
			vq->restart_count = restart_count;
			vq->state = VAL_INIT_STATE;
			verbose(VERB_ALGO, "pass back to next module");
			qstate->ext_state[id] = module_restart_next;
			return 0;
		}

		vq->orig_msg->rep->ttl = ve->bogus_ttl;
		vq->orig_msg->rep->prefetch_ttl = 
			PREFETCH_TTL_CALC(vq->orig_msg->rep->ttl);
		vq->orig_msg->rep->serve_expired_ttl = 
			vq->orig_msg->rep->ttl + qstate->env->cfg->serve_expired_ttl;
		if((qstate->env->cfg->val_log_level >= 1 ||
			qstate->env->cfg->log_servfail) &&
			!qstate->env->cfg->val_log_squelch) {
			if(qstate->env->cfg->val_log_level < 2 &&
				!qstate->env->cfg->log_servfail)
				log_query_info(NO_VERBOSE, "validation failure",
					&qstate->qinfo);
			else {
				char* err = errinf_to_str_bogus(qstate);
				if(err) log_info("%s", err);
				free(err);
			}
		}
		/*
		 * If set, the validator will not make messages bogus, instead
		 * indeterminate is issued, so that no clients receive SERVFAIL.
		 * This allows an operator to run validation 'shadow' without
		 * hurting responses to clients.
		 */
		/* If we are in permissive mode, bogus gets indeterminate */
		if(qstate->env->cfg->val_permissive_mode)
			vq->orig_msg->rep->security = sec_status_indeterminate;
	}

	if(vq->orig_msg->rep->security == sec_status_secure &&
		qstate->env->cfg->root_key_sentinel &&
		(qstate->qinfo.qtype == LDNS_RR_TYPE_A ||
		qstate->qinfo.qtype == LDNS_RR_TYPE_AAAA)) {
		char* keytag_start;
		uint16_t keytag;
		if(*qstate->qinfo.qname == strlen(SENTINEL_IS) +
			SENTINEL_KEYTAG_LEN &&
			dname_lab_startswith(qstate->qinfo.qname, SENTINEL_IS,
			&keytag_start)) {
			if(sentinel_get_keytag(keytag_start, &keytag) &&
				!anchor_has_keytag(qstate->env->anchors,
				(uint8_t*)"", 1, 0, vq->qchase.qclass, keytag)) {
				vq->orig_msg->rep->security =
					sec_status_secure_sentinel_fail;
			}
		} else if(*qstate->qinfo.qname == strlen(SENTINEL_NOT) +
			SENTINEL_KEYTAG_LEN &&
			dname_lab_startswith(qstate->qinfo.qname, SENTINEL_NOT,
			&keytag_start)) {
			if(sentinel_get_keytag(keytag_start, &keytag) &&
				anchor_has_keytag(qstate->env->anchors,
				(uint8_t*)"", 1, 0, vq->qchase.qclass, keytag)) {
				vq->orig_msg->rep->security =
					sec_status_secure_sentinel_fail;
			}
		}
	}
	/* store results in cache */
	if(qstate->query_flags&BIT_RD) {
		/* if secure, this will override cache anyway, no need
		 * to check if from parentNS */
		if(!qstate->no_cache_store) {
			if(!dns_cache_store(qstate->env, &vq->orig_msg->qinfo,
				vq->orig_msg->rep, 0, qstate->prefetch_leeway, 0, NULL,
				qstate->query_flags)) {
				log_err("out of memory caching validator results");
			}
		}
	} else {
		/* for a referral, store the verified RRsets */
		/* and this does not get prefetched, so no leeway */
		if(!dns_cache_store(qstate->env, &vq->orig_msg->qinfo,
			vq->orig_msg->rep, 1, 0, 0, NULL,
			qstate->query_flags)) {
			log_err("out of memory caching validator results");
		}
	}
	qstate->return_rcode = LDNS_RCODE_NOERROR;
	qstate->return_msg = vq->orig_msg;
	qstate->ext_state[id] = module_finished;
	return 0;
}