processLastResort(struct module_qstate* qstate, struct iter_qstate* iq,
	struct iter_env* ie, int id)
{
	struct delegpt_ns* ns;
	int query_count = 0;
	verbose(VERB_ALGO, "No more query targets, attempting last resort");
	log_assert(iq->dp);

	if(!can_have_last_resort(qstate->env, iq->dp->name, iq->dp->namelen,
		iq->qchase.qclass, NULL)) {
		/* fail -- no more targets, no more hope of targets, no hope 
		 * of a response. */
		errinf(qstate, "all the configured stub or forward servers failed,");
		errinf_dname(qstate, "at zone", iq->dp->name);
		verbose(VERB_QUERY, "configured stub or forward servers failed -- returning SERVFAIL");
		return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	if(!iq->dp->has_parent_side_NS && dname_is_root(iq->dp->name)) {
		struct delegpt* p = hints_lookup_root(qstate->env->hints,
			iq->qchase.qclass);
		if(p) {
			struct delegpt_addr* a;
			iq->chase_flags &= ~BIT_RD; /* go to authorities */
			for(ns = p->nslist; ns; ns=ns->next) {
				(void)delegpt_add_ns(iq->dp, qstate->region,
					ns->name, ns->lame);
			}
			for(a = p->target_list; a; a=a->next_target) {
				(void)delegpt_add_addr(iq->dp, qstate->region,
					&a->addr, a->addrlen, a->bogus,
					a->lame, a->tls_auth_name);
			}
		}
		iq->dp->has_parent_side_NS = 1;
	} else if(!iq->dp->has_parent_side_NS) {
		if(!iter_lookup_parent_NS_from_cache(qstate->env, iq->dp,
			qstate->region, &qstate->qinfo) 
			|| !iq->dp->has_parent_side_NS) {
			/* if: malloc failure in lookup go up to try */
			/* if: no parent NS in cache - go up one level */
			verbose(VERB_ALGO, "try to grab parent NS");
			iq->store_parent_NS = iq->dp;
			iq->chase_flags &= ~BIT_RD; /* go to authorities */
			iq->deleg_msg = NULL;
			iq->refetch_glue = 1;
			iq->query_restart_count++;
			iq->sent_count = 0;
			if(qstate->env->cfg->qname_minimisation)
				iq->minimisation_state = INIT_MINIMISE_STATE;
			return next_state(iq, INIT_REQUEST_STATE);
		}
	}
	/* see if that makes new names available */
	if(!cache_fill_missing(qstate->env, iq->qchase.qclass, 
		qstate->region, iq->dp))
		log_err("out of memory in cache_fill_missing");
	if(iq->dp->usable_list) {
		verbose(VERB_ALGO, "try parent-side-name, w. glue from cache");
		return next_state(iq, QUERYTARGETS_STATE);
	}
	/* try to fill out parent glue from cache */
	if(iter_lookup_parent_glue_from_cache(qstate->env, iq->dp,
		qstate->region, &qstate->qinfo)) {
		/* got parent stuff from cache, see if we can continue */
		verbose(VERB_ALGO, "try parent-side glue from cache");
		return next_state(iq, QUERYTARGETS_STATE);
	}
	/* query for an extra name added by the parent-NS record */
	if(delegpt_count_missing_targets(iq->dp) > 0) {
		int qs = 0;
		verbose(VERB_ALGO, "try parent-side target name");
		if(!query_for_targets(qstate, iq, ie, id, 1, &qs)) {
			errinf(qstate, "could not fetch nameserver");
			errinf_dname(qstate, "at zone", iq->dp->name);
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		iq->num_target_queries += qs;
		target_count_increase(iq, qs);
		if(qs != 0) {
			qstate->ext_state[id] = module_wait_subquery;
			return 0; /* and wait for them */
		}
	}
	if(iq->depth == ie->max_dependency_depth) {
		verbose(VERB_QUERY, "maxdepth and need more nameservers, fail");
		errinf(qstate, "cannot fetch more nameservers because at max dependency depth");
		return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	if(iq->depth > 0 && iq->target_count &&
		iq->target_count[1] > MAX_TARGET_COUNT) {
		char s[LDNS_MAX_DOMAINLEN+1];
		dname_str(qstate->qinfo.qname, s);
		verbose(VERB_QUERY, "request %s has exceeded the maximum "
			"number of glue fetches %d", s, iq->target_count[1]);
		errinf(qstate, "exceeded the maximum number of glue fetches");
		return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
	}
	/* mark cycle targets for parent-side lookups */
	iter_mark_pside_cycle_targets(qstate, iq->dp);
	/* see if we can issue queries to get nameserver addresses */
	/* this lookup is not randomized, but sequential. */
	for(ns = iq->dp->nslist; ns; ns = ns->next) {
		/* if this nameserver is at a delegation point, but that
		 * delegation point is a stub and we cannot go higher, skip*/
		if( ((ie->supports_ipv6 && !ns->done_pside6) ||
		    (ie->supports_ipv4 && !ns->done_pside4)) &&
		    !can_have_last_resort(qstate->env, ns->name, ns->namelen,
			iq->qchase.qclass, NULL)) {
			log_nametypeclass(VERB_ALGO, "cannot pside lookup ns "
				"because it is also a stub/forward,",
				ns->name, LDNS_RR_TYPE_NS, iq->qchase.qclass);
			if(ie->supports_ipv6) ns->done_pside6 = 1;
			if(ie->supports_ipv4) ns->done_pside4 = 1;
			continue;
		}
		/* query for parent-side A and AAAA for nameservers */
		if(ie->supports_ipv6 && !ns->done_pside6) {
			/* Send the AAAA request. */
			if(!generate_parentside_target_query(qstate, iq, id, 
				ns->name, ns->namelen,
				LDNS_RR_TYPE_AAAA, iq->qchase.qclass)) {
				errinf_dname(qstate, "could not generate nameserver AAAA lookup for", ns->name);
				return error_response(qstate, id,
					LDNS_RCODE_SERVFAIL);
			}
			ns->done_pside6 = 1;
			query_count++;
		}
		if(ie->supports_ipv4 && !ns->done_pside4) {
			/* Send the A request. */
			if(!generate_parentside_target_query(qstate, iq, id, 
				ns->name, ns->namelen, 
				LDNS_RR_TYPE_A, iq->qchase.qclass)) {
				errinf_dname(qstate, "could not generate nameserver A lookup for", ns->name);
				return error_response(qstate, id,
					LDNS_RCODE_SERVFAIL);
			}
			ns->done_pside4 = 1;
			query_count++;
		}
		if(query_count != 0) { /* suspend to await results */
			verbose(VERB_ALGO, "try parent-side glue lookup");
			iq->num_target_queries += query_count;
			target_count_increase(iq, query_count);
			qstate->ext_state[id] = module_wait_subquery;
			return 0;
		}
	}

	/* if this was a parent-side glue query itself, then store that
	 * failure in cache. */
	if(!qstate->no_cache_store && iq->query_for_pside_glue
		&& !iq->pside_glue)
			iter_store_parentside_neg(qstate->env, &qstate->qinfo,
				iq->deleg_msg?iq->deleg_msg->rep:
				(iq->response?iq->response->rep:NULL));

	errinf(qstate, "all servers for this domain failed,");
	errinf_dname(qstate, "at zone", iq->dp->name);
	verbose(VERB_QUERY, "out of query targets -- returning SERVFAIL");
	/* fail -- no more targets, no more hope of targets, no hope 
	 * of a response. */
	return error_response_cache(qstate, id, LDNS_RCODE_SERVFAIL);
}