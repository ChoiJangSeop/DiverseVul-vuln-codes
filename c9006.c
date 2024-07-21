processQueryTargets(struct module_qstate* qstate, struct iter_qstate* iq,
	struct iter_env* ie, int id)
{
	int tf_policy;
	struct delegpt_addr* target;
	struct outbound_entry* outq;
	int auth_fallback = 0;
	uint8_t* qout_orig = NULL;
	size_t qout_orig_len = 0;

	/* NOTE: a request will encounter this state for each target it 
	 * needs to send a query to. That is, at least one per referral, 
	 * more if some targets timeout or return throwaway answers. */

	log_query_info(VERB_QUERY, "processQueryTargets:", &qstate->qinfo);
	verbose(VERB_ALGO, "processQueryTargets: targetqueries %d, "
		"currentqueries %d sentcount %d", iq->num_target_queries, 
		iq->num_current_queries, iq->sent_count);

	/* Make sure that we haven't run away */
	/* FIXME: is this check even necessary? */
	if(iq->referral_count > MAX_REFERRAL_COUNT) {
		verbose(VERB_QUERY, "request has exceeded the maximum "
			"number of referrrals with %d", iq->referral_count);
		errinf(qstate, "exceeded the maximum of referrals");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	if(iq->sent_count > MAX_SENT_COUNT) {
		verbose(VERB_QUERY, "request has exceeded the maximum "
			"number of sends with %d", iq->sent_count);
		errinf(qstate, "exceeded the maximum number of sends");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	
	/* Make sure we have a delegation point, otherwise priming failed
	 * or another failure occurred */
	if(!iq->dp) {
		verbose(VERB_QUERY, "Failed to get a delegation, giving up");
		errinf(qstate, "failed to get a delegation (eg. prime failure)");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	if(!ie->supports_ipv6)
		delegpt_no_ipv6(iq->dp);
	if(!ie->supports_ipv4)
		delegpt_no_ipv4(iq->dp);
	delegpt_log(VERB_ALGO, iq->dp);

	if(iq->num_current_queries>0) {
		/* already busy answering a query, this restart is because
		 * more delegpt addrs became available, wait for existing
		 * query. */
		verbose(VERB_ALGO, "woke up, but wait for outstanding query");
		qstate->ext_state[id] = module_wait_reply;
		return 0;
	}

	if(iq->minimisation_state == INIT_MINIMISE_STATE
		&& !(iq->chase_flags & BIT_RD)) {
		/* (Re)set qinfo_out to (new) delegation point, except when
		 * qinfo_out is already a subdomain of dp. This happens when
		 * increasing by more than one label at once (QNAMEs with more
		 * than MAX_MINIMISE_COUNT labels). */
		if(!(iq->qinfo_out.qname_len 
			&& dname_subdomain_c(iq->qchase.qname, 
				iq->qinfo_out.qname)
			&& dname_subdomain_c(iq->qinfo_out.qname, 
				iq->dp->name))) {
			iq->qinfo_out.qname = iq->dp->name;
			iq->qinfo_out.qname_len = iq->dp->namelen;
			iq->qinfo_out.qtype = LDNS_RR_TYPE_A;
			iq->qinfo_out.qclass = iq->qchase.qclass;
			iq->qinfo_out.local_alias = NULL;
			iq->minimise_count = 0;
		}

		iq->minimisation_state = MINIMISE_STATE;
	}
	if(iq->minimisation_state == MINIMISE_STATE) {
		int qchaselabs = dname_count_labels(iq->qchase.qname);
		int labdiff = qchaselabs -
			dname_count_labels(iq->qinfo_out.qname);

		qout_orig = iq->qinfo_out.qname;
		qout_orig_len = iq->qinfo_out.qname_len;
		iq->qinfo_out.qname = iq->qchase.qname;
		iq->qinfo_out.qname_len = iq->qchase.qname_len;
		iq->minimise_count++;
		iq->timeout_count = 0;

		iter_dec_attempts(iq->dp, 1);

		/* Limit number of iterations for QNAMEs with more
		 * than MAX_MINIMISE_COUNT labels. Send first MINIMISE_ONE_LAB
		 * labels of QNAME always individually.
		 */
		if(qchaselabs > MAX_MINIMISE_COUNT && labdiff > 1 && 
			iq->minimise_count > MINIMISE_ONE_LAB) {
			if(iq->minimise_count < MAX_MINIMISE_COUNT) {
				int multilabs = qchaselabs - 1 - 
					MINIMISE_ONE_LAB;
				int extralabs = multilabs / 
					MINIMISE_MULTIPLE_LABS;

				if (MAX_MINIMISE_COUNT - iq->minimise_count >= 
					multilabs % MINIMISE_MULTIPLE_LABS)
					/* Default behaviour is to add 1 label
					 * every iteration. Therefore, decrement
					 * the extralabs by 1 */
					extralabs--;
				if (extralabs < labdiff)
					labdiff -= extralabs;
				else
					labdiff = 1;
			}
			/* Last minimised iteration, send all labels with
			 * QTYPE=NS */
			else
				labdiff = 1;
		}

		if(labdiff > 1) {
			verbose(VERB_QUERY, "removing %d labels", labdiff-1);
			dname_remove_labels(&iq->qinfo_out.qname, 
				&iq->qinfo_out.qname_len, 
				labdiff-1);
		}
		if(labdiff < 1 || (labdiff < 2 
			&& (iq->qchase.qtype == LDNS_RR_TYPE_DS
			|| iq->qchase.qtype == LDNS_RR_TYPE_A)))
			/* Stop minimising this query, resolve "as usual" */
			iq->minimisation_state = DONOT_MINIMISE_STATE;
		else if(!qstate->no_cache_lookup) {
			struct dns_msg* msg = dns_cache_lookup(qstate->env, 
				iq->qinfo_out.qname, iq->qinfo_out.qname_len, 
				iq->qinfo_out.qtype, iq->qinfo_out.qclass, 
				qstate->query_flags, qstate->region, 
				qstate->env->scratch, 0);
			if(msg && msg->rep->an_numrrsets == 0
				&& FLAGS_GET_RCODE(msg->rep->flags) == 
				LDNS_RCODE_NOERROR)
				/* no need to send query if it is already 
				 * cached as NOERROR/NODATA */
				return 1;
		}
	}
	if(iq->minimisation_state == SKIP_MINIMISE_STATE) {
		if(iq->timeout_count < MAX_MINIMISE_TIMEOUT_COUNT)
			/* Do not increment qname, continue incrementing next 
			 * iteration */
			iq->minimisation_state = MINIMISE_STATE;
		else if(!qstate->env->cfg->qname_minimisation_strict)
			/* Too many time-outs detected for this QNAME and QTYPE.
			 * We give up, disable QNAME minimisation. */
			iq->minimisation_state = DONOT_MINIMISE_STATE;
	}
	if(iq->minimisation_state == DONOT_MINIMISE_STATE)
		iq->qinfo_out = iq->qchase;

	/* now find an answer to this query */
	/* see if authority zones have an answer */
	/* now we know the dp, we can check the auth zone for locally hosted
	 * contents */
	if(!iq->auth_zone_avoid && qstate->blacklist) {
		if(auth_zones_can_fallback(qstate->env->auth_zones,
			iq->dp->name, iq->dp->namelen, iq->qinfo_out.qclass)) {
			/* if cache is blacklisted and this zone allows us
			 * to fallback to the internet, then do so, and
			 * fetch results from the internet servers */
			iq->auth_zone_avoid = 1;
		}
	}
	if(iq->auth_zone_avoid) {
		iq->auth_zone_avoid = 0;
		auth_fallback = 1;
	} else if(auth_zones_lookup(qstate->env->auth_zones, &iq->qinfo_out,
		qstate->region, &iq->response, &auth_fallback, iq->dp->name,
		iq->dp->namelen)) {
		/* use this as a response to be processed by the iterator */
		if(verbosity >= VERB_ALGO) {
			log_dns_msg("msg from auth zone",
				&iq->response->qinfo, iq->response->rep);
		}
		if((iq->chase_flags&BIT_RD) && !(iq->response->rep->flags&BIT_AA)) {
			verbose(VERB_ALGO, "forwarder, ignoring referral from auth zone");
		} else {
			lock_rw_wrlock(&qstate->env->auth_zones->lock);
			qstate->env->auth_zones->num_query_up++;
			lock_rw_unlock(&qstate->env->auth_zones->lock);
			iq->num_current_queries++;
			iq->chase_to_rd = 0;
			iq->dnssec_lame_query = 0;
			iq->auth_zone_response = 1;
			return next_state(iq, QUERY_RESP_STATE);
		}
	}
	iq->auth_zone_response = 0;
	if(auth_fallback == 0) {
		/* like we got servfail from the auth zone lookup, and
		 * no internet fallback */
		verbose(VERB_ALGO, "auth zone lookup failed, no fallback,"
			" servfail");
		errinf(qstate, "auth zone lookup failed, fallback is off");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	if(iq->dp->auth_dp) {
		/* we wanted to fallback, but had no delegpt, only the
		 * auth zone generated delegpt, create an actual one */
		iq->auth_zone_avoid = 1;
		return next_state(iq, INIT_REQUEST_STATE);
	}
	/* but mostly, fallback==1 (like, when no such auth zone exists)
	 * and we continue with lookups */

	tf_policy = 0;
	/* < not <=, because although the array is large enough for <=, the
	 * generated query will immediately be discarded due to depth and
	 * that servfail is cached, which is not good as opportunism goes. */
	if(iq->depth < ie->max_dependency_depth
		&& iq->sent_count < TARGET_FETCH_STOP) {
		tf_policy = ie->target_fetch_policy[iq->depth];
	}

	/* if in 0x20 fallback get as many targets as possible */
	if(iq->caps_fallback) {
		int extra = 0;
		size_t naddr, nres, navail;
		if(!query_for_targets(qstate, iq, ie, id, -1, &extra)) {
			errinf(qstate, "could not fetch nameservers for 0x20 fallback");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		iq->num_target_queries += extra;
		target_count_increase(iq, extra);
		if(iq->num_target_queries > 0) {
			/* wait to get all targets, we want to try em */
			verbose(VERB_ALGO, "wait for all targets for fallback");
			qstate->ext_state[id] = module_wait_reply;
			/* undo qname minimise step because we'll get back here
			 * to do it again */
			if(qout_orig && iq->minimise_count > 0) {
				iq->minimise_count--;
				iq->qinfo_out.qname = qout_orig;
				iq->qinfo_out.qname_len = qout_orig_len;
			}
			return 0;
		}
		/* did we do enough fallback queries already? */
		delegpt_count_addr(iq->dp, &naddr, &nres, &navail);
		/* the current caps_server is the number of fallbacks sent.
		 * the original query is one that matched too, so we have
		 * caps_server+1 number of matching queries now */
		if(iq->caps_server+1 >= naddr*3 ||
			iq->caps_server*2+2 >= MAX_SENT_COUNT) {
			/* *2 on sentcount check because ipv6 may fail */
			/* we're done, process the response */
			verbose(VERB_ALGO, "0x20 fallback had %d responses "
				"match for %d wanted, done.", 
				(int)iq->caps_server+1, (int)naddr*3);
			iq->response = iq->caps_response;
			iq->caps_fallback = 0;
			iter_dec_attempts(iq->dp, 3); /* space for fallback */
			iq->num_current_queries++; /* RespState decrements it*/
			iq->referral_count++; /* make sure we don't loop */
			iq->sent_count = 0;
			iq->state = QUERY_RESP_STATE;
			return 1;
		}
		verbose(VERB_ALGO, "0x20 fallback number %d", 
			(int)iq->caps_server);

	/* if there is a policy to fetch missing targets 
	 * opportunistically, do it. we rely on the fact that once a 
	 * query (or queries) for a missing name have been issued, 
	 * they will not show up again. */
	} else if(tf_policy != 0) {
		int extra = 0;
		verbose(VERB_ALGO, "attempt to get extra %d targets", 
			tf_policy);
		(void)query_for_targets(qstate, iq, ie, id, tf_policy, &extra);
		/* errors ignored, these targets are not strictly necessary for
		 * this result, we do not have to reply with SERVFAIL */
		iq->num_target_queries += extra;
		target_count_increase(iq, extra);
	}

	/* Add the current set of unused targets to our queue. */
	delegpt_add_unused_targets(iq->dp);

	/* Select the next usable target, filtering out unsuitable targets. */
	target = iter_server_selection(ie, qstate->env, iq->dp, 
		iq->dp->name, iq->dp->namelen, iq->qchase.qtype,
		&iq->dnssec_lame_query, &iq->chase_to_rd, 
		iq->num_target_queries, qstate->blacklist,
		qstate->prefetch_leeway);

	/* If no usable target was selected... */
	if(!target) {
		/* Here we distinguish between three states: generate a new 
		 * target query, just wait, or quit (with a SERVFAIL).
		 * We have the following information: number of active 
		 * target queries, number of active current queries, 
		 * the presence of missing targets at this delegation 
		 * point, and the given query target policy. */
		
		/* Check for the wait condition. If this is true, then 
		 * an action must be taken. */
		if(iq->num_target_queries==0 && iq->num_current_queries==0) {
			/* If there is nothing to wait for, then we need 
			 * to distinguish between generating (a) new target 
			 * query, or failing. */
			if(delegpt_count_missing_targets(iq->dp) > 0) {
				int qs = 0;
				verbose(VERB_ALGO, "querying for next "
					"missing target");
				if(!query_for_targets(qstate, iq, ie, id, 
					1, &qs)) {
					errinf(qstate, "could not fetch nameserver");
					errinf_dname(qstate, "at zone", iq->dp->name);
					return error_response(qstate, id,
						LDNS_RCODE_SERVFAIL);
				}
				if(qs == 0 && 
				   delegpt_count_missing_targets(iq->dp) == 0){
					/* it looked like there were missing
					 * targets, but they did not turn up.
					 * Try the bad choices again (if any),
					 * when we get back here missing==0,
					 * so this is not a loop. */
					return 1;
				}
				iq->num_target_queries += qs;
				target_count_increase(iq, qs);
			}
			/* Since a target query might have been made, we 
			 * need to check again. */
			if(iq->num_target_queries == 0) {
				/* if in capsforid fallback, instead of last
				 * resort, we agree with the current reply
				 * we have (if any) (our count of addrs bad)*/
				if(iq->caps_fallback && iq->caps_reply) {
					/* we're done, process the response */
					verbose(VERB_ALGO, "0x20 fallback had %d responses, "
						"but no more servers except "
						"last resort, done.", 
						(int)iq->caps_server+1);
					iq->response = iq->caps_response;
					iq->caps_fallback = 0;
					iter_dec_attempts(iq->dp, 3); /* space for fallback */
					iq->num_current_queries++; /* RespState decrements it*/
					iq->referral_count++; /* make sure we don't loop */
					iq->sent_count = 0;
					iq->state = QUERY_RESP_STATE;
					return 1;
				}
				return processLastResort(qstate, iq, ie, id);
			}
		}

		/* otherwise, we have no current targets, so submerge 
		 * until one of the target or direct queries return. */
		if(iq->num_target_queries>0 && iq->num_current_queries>0) {
			verbose(VERB_ALGO, "no current targets -- waiting "
				"for %d targets to resolve or %d outstanding"
				" queries to respond", iq->num_target_queries, 
				iq->num_current_queries);
			qstate->ext_state[id] = module_wait_reply;
		} else if(iq->num_target_queries>0) {
			verbose(VERB_ALGO, "no current targets -- waiting "
				"for %d targets to resolve.",
				iq->num_target_queries);
			qstate->ext_state[id] = module_wait_subquery;
		} else {
			verbose(VERB_ALGO, "no current targets -- waiting "
				"for %d outstanding queries to respond.",
				iq->num_current_queries);
			qstate->ext_state[id] = module_wait_reply;
		}
		/* undo qname minimise step because we'll get back here
		 * to do it again */
		if(qout_orig && iq->minimise_count > 0) {
			iq->minimise_count--;
			iq->qinfo_out.qname = qout_orig;
			iq->qinfo_out.qname_len = qout_orig_len;
		}
		return 0;
	}

	/* if not forwarding, check ratelimits per delegationpoint name */
	if(!(iq->chase_flags & BIT_RD) && !iq->ratelimit_ok) {
		if(!infra_ratelimit_inc(qstate->env->infra_cache, iq->dp->name,
			iq->dp->namelen, *qstate->env->now, &qstate->qinfo,
			qstate->reply)) {
			lock_basic_lock(&ie->queries_ratelimit_lock);
			ie->num_queries_ratelimited++;
			lock_basic_unlock(&ie->queries_ratelimit_lock);
			verbose(VERB_ALGO, "query exceeded ratelimits");
			qstate->was_ratelimited = 1;
			errinf_dname(qstate, "exceeded ratelimit for zone",
				iq->dp->name);
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
	}

	/* We have a valid target. */
	if(verbosity >= VERB_QUERY) {
		log_query_info(VERB_QUERY, "sending query:", &iq->qinfo_out);
		log_name_addr(VERB_QUERY, "sending to target:", iq->dp->name, 
			&target->addr, target->addrlen);
		verbose(VERB_ALGO, "dnssec status: %s%s",
			iq->dnssec_expected?"expected": "not expected",
			iq->dnssec_lame_query?" but lame_query anyway": "");
	}
	fptr_ok(fptr_whitelist_modenv_send_query(qstate->env->send_query));
	outq = (*qstate->env->send_query)(&iq->qinfo_out,
		iq->chase_flags | (iq->chase_to_rd?BIT_RD:0), 
		/* unset CD if to forwarder(RD set) and not dnssec retry
		 * (blacklist nonempty) and no trust-anchors are configured
		 * above the qname or on the first attempt when dnssec is on */
		EDNS_DO| ((iq->chase_to_rd||(iq->chase_flags&BIT_RD)!=0)&&
		!qstate->blacklist&&(!iter_qname_indicates_dnssec(qstate->env,
		&iq->qinfo_out)||target->attempts==1)?0:BIT_CD), 
		iq->dnssec_expected, iq->caps_fallback || is_caps_whitelisted(
		ie, iq), &target->addr, target->addrlen,
		iq->dp->name, iq->dp->namelen,
		(iq->dp->ssl_upstream || qstate->env->cfg->ssl_upstream),
		target->tls_auth_name, qstate);
	if(!outq) {
		log_addr(VERB_DETAIL, "error sending query to auth server", 
			&target->addr, target->addrlen);
		if(!(iq->chase_flags & BIT_RD) && !iq->ratelimit_ok)
		    infra_ratelimit_dec(qstate->env->infra_cache, iq->dp->name,
			iq->dp->namelen, *qstate->env->now);
		if(qstate->env->cfg->qname_minimisation)
			iq->minimisation_state = SKIP_MINIMISE_STATE;
		return next_state(iq, QUERYTARGETS_STATE);
	}
	outbound_list_insert(&iq->outlist, outq);
	iq->num_current_queries++;
	iq->sent_count++;
	qstate->ext_state[id] = module_wait_reply;

	return 0;
}