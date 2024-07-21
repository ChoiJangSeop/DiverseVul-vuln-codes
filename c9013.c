processInitRequest(struct module_qstate* qstate, struct iter_qstate* iq,
	struct iter_env* ie, int id)
{
	uint8_t* delname;
	size_t delnamelen;
	struct dns_msg* msg = NULL;

	log_query_info(VERB_DETAIL, "resolving", &qstate->qinfo);
	/* check effort */

	/* We enforce a maximum number of query restarts. This is primarily a
	 * cheap way to prevent CNAME loops. */
	if(iq->query_restart_count > MAX_RESTART_COUNT) {
		verbose(VERB_QUERY, "request has exceeded the maximum number"
			" of query restarts with %d", iq->query_restart_count);
		errinf(qstate, "request has exceeded the maximum number "
			"restarts (eg. indirections)");
		if(iq->qchase.qname)
			errinf_dname(qstate, "stop at", iq->qchase.qname);
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}

	/* We enforce a maximum recursion/dependency depth -- in general, 
	 * this is unnecessary for dependency loops (although it will 
	 * catch those), but it provides a sensible limit to the amount 
	 * of work required to answer a given query. */
	verbose(VERB_ALGO, "request has dependency depth of %d", iq->depth);
	if(iq->depth > ie->max_dependency_depth) {
		verbose(VERB_QUERY, "request has exceeded the maximum "
			"dependency depth with depth of %d", iq->depth);
		errinf(qstate, "request has exceeded the maximum dependency "
			"depth (eg. nameserver lookup recursion)");
		return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
	}

	/* If the request is qclass=ANY, setup to generate each class */
	if(qstate->qinfo.qclass == LDNS_RR_CLASS_ANY) {
		iq->qchase.qclass = 0;
		return next_state(iq, COLLECT_CLASS_STATE);
	}

	/*
	 * If we are restricted by a forward-zone or a stub-zone, we
	 * can't re-fetch glue for this delegation point.
	 * we won’t try to re-fetch glue if the iq->dp is null.
	 */
	if (iq->refetch_glue &&
	        iq->dp &&
	        !can_have_last_resort(qstate->env, iq->dp->name,
	             iq->dp->namelen, iq->qchase.qclass, NULL)) {
	    iq->refetch_glue = 0;
	}

	/* Resolver Algorithm Step 1 -- Look for the answer in local data. */

	/* This either results in a query restart (CNAME cache response), a
	 * terminating response (ANSWER), or a cache miss (null). */
	
	if (iter_stub_fwd_no_cache(qstate, &iq->qchase)) {
		/* Asked to not query cache. */
		verbose(VERB_ALGO, "no-cache set, going to the network");
		qstate->no_cache_lookup = 1;
		qstate->no_cache_store = 1;
		msg = NULL;
	} else if(qstate->blacklist) {
		/* if cache, or anything else, was blacklisted then
		 * getting older results from cache is a bad idea, no cache */
		verbose(VERB_ALGO, "cache blacklisted, going to the network");
		msg = NULL;
	} else if(!qstate->no_cache_lookup) {
		msg = dns_cache_lookup(qstate->env, iq->qchase.qname, 
			iq->qchase.qname_len, iq->qchase.qtype, 
			iq->qchase.qclass, qstate->query_flags,
			qstate->region, qstate->env->scratch, 0);
		if(!msg && qstate->env->neg_cache &&
			iter_qname_indicates_dnssec(qstate->env, &iq->qchase)) {
			/* lookup in negative cache; may result in
			 * NOERROR/NODATA or NXDOMAIN answers that need validation */
			msg = val_neg_getmsg(qstate->env->neg_cache, &iq->qchase,
				qstate->region, qstate->env->rrset_cache,
				qstate->env->scratch_buffer, 
				*qstate->env->now, 1/*add SOA*/, NULL, 
				qstate->env->cfg);
		}
		/* item taken from cache does not match our query name, thus
		 * security needs to be re-examined later */
		if(msg && query_dname_compare(qstate->qinfo.qname,
			iq->qchase.qname) != 0)
			msg->rep->security = sec_status_unchecked;
	}
	if(msg) {
		/* handle positive cache response */
		enum response_type type = response_type_from_cache(msg, 
			&iq->qchase);
		if(verbosity >= VERB_ALGO) {
			log_dns_msg("msg from cache lookup", &msg->qinfo, 
				msg->rep);
			verbose(VERB_ALGO, "msg ttl is %d, prefetch ttl %d", 
				(int)msg->rep->ttl, 
				(int)msg->rep->prefetch_ttl);
		}

		if(type == RESPONSE_TYPE_CNAME) {
			uint8_t* sname = 0;
			size_t slen = 0;
			verbose(VERB_ALGO, "returning CNAME response from "
				"cache");
			if(!handle_cname_response(qstate, iq, msg, 
				&sname, &slen)) {
				errinf(qstate, "failed to prepend CNAME "
					"components, malloc failure");
				return error_response(qstate, id, 
					LDNS_RCODE_SERVFAIL);
			}
			iq->qchase.qname = sname;
			iq->qchase.qname_len = slen;
			/* This *is* a query restart, even if it is a cheap 
			 * one. */
			iq->dp = NULL;
			iq->refetch_glue = 0;
			iq->query_restart_count++;
			iq->sent_count = 0;
			sock_list_insert(&qstate->reply_origin, NULL, 0, qstate->region);
			if(qstate->env->cfg->qname_minimisation)
				iq->minimisation_state = INIT_MINIMISE_STATE;
			return next_state(iq, INIT_REQUEST_STATE);
		}

		/* if from cache, NULL, else insert 'cache IP' len=0 */
		if(qstate->reply_origin)
			sock_list_insert(&qstate->reply_origin, NULL, 0, qstate->region);
		if(FLAGS_GET_RCODE(msg->rep->flags) == LDNS_RCODE_SERVFAIL)
			errinf(qstate, "SERVFAIL in cache");
		/* it is an answer, response, to final state */
		verbose(VERB_ALGO, "returning answer from cache.");
		iq->response = msg;
		return final_state(iq);
	}

	/* attempt to forward the request */
	if(forward_request(qstate, iq))
	{
		if(!iq->dp) {
			log_err("alloc failure for forward dp");
			errinf(qstate, "malloc failure for forward zone");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		iq->refetch_glue = 0;
		iq->minimisation_state = DONOT_MINIMISE_STATE;
		/* the request has been forwarded.
		 * forwarded requests need to be immediately sent to the 
		 * next state, QUERYTARGETS. */
		return next_state(iq, QUERYTARGETS_STATE);
	}

	/* Resolver Algorithm Step 2 -- find the "best" servers. */

	/* first, adjust for DS queries. To avoid the grandparent problem, 
	 * we just look for the closest set of server to the parent of qname.
	 * When re-fetching glue we also need to ask the parent.
	 */
	if(iq->refetch_glue) {
		if(!iq->dp) {
			log_err("internal or malloc fail: no dp for refetch");
			errinf(qstate, "malloc failure, for delegation info");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		delname = iq->dp->name;
		delnamelen = iq->dp->namelen;
	} else {
		delname = iq->qchase.qname;
		delnamelen = iq->qchase.qname_len;
	}
	if(iq->qchase.qtype == LDNS_RR_TYPE_DS || iq->refetch_glue ||
	   (iq->qchase.qtype == LDNS_RR_TYPE_NS && qstate->prefetch_leeway
	   && can_have_last_resort(qstate->env, delname, delnamelen, iq->qchase.qclass, NULL))) {
		/* remove first label from delname, root goes to hints,
		 * but only to fetch glue, not for qtype=DS. */
		/* also when prefetching an NS record, fetch it again from
		 * its parent, just as if it expired, so that you do not
		 * get stuck on an older nameserver that gives old NSrecords */
		if(dname_is_root(delname) && (iq->refetch_glue ||
			(iq->qchase.qtype == LDNS_RR_TYPE_NS &&
			qstate->prefetch_leeway)))
			delname = NULL; /* go to root priming */
		else 	dname_remove_label(&delname, &delnamelen);
	}
	/* delname is the name to lookup a delegation for. If NULL rootprime */
	while(1) {
		
		/* Lookup the delegation in the cache. If null, then the 
		 * cache needs to be primed for the qclass. */
		if(delname)
		     iq->dp = dns_cache_find_delegation(qstate->env, delname, 
			delnamelen, iq->qchase.qtype, iq->qchase.qclass, 
			qstate->region, &iq->deleg_msg,
			*qstate->env->now+qstate->prefetch_leeway);
		else iq->dp = NULL;

		/* If the cache has returned nothing, then we have a 
		 * root priming situation. */
		if(iq->dp == NULL) {
			int r;
			/* if under auth zone, no prime needed */
			if(!auth_zone_delegpt(qstate, iq, delname, delnamelen))
				return error_response(qstate, id, 
					LDNS_RCODE_SERVFAIL);
			if(iq->dp) /* use auth zone dp */
				return next_state(iq, INIT_REQUEST_2_STATE);
			/* if there is a stub, then no root prime needed */
			r = prime_stub(qstate, iq, id, delname,
				iq->qchase.qclass);
			if(r == 2)
				break; /* got noprime-stub-zone, continue */
			else if(r)
				return 0; /* stub prime request made */
			if(forwards_lookup_root(qstate->env->fwds, 
				iq->qchase.qclass)) {
				/* forward zone root, no root prime needed */
				/* fill in some dp - safety belt */
				iq->dp = hints_lookup_root(qstate->env->hints, 
					iq->qchase.qclass);
				if(!iq->dp) {
					log_err("internal error: no hints dp");
					errinf(qstate, "no hints for this class");
					return error_response(qstate, id, 
						LDNS_RCODE_SERVFAIL);
				}
				iq->dp = delegpt_copy(iq->dp, qstate->region);
				if(!iq->dp) {
					log_err("out of memory in safety belt");
					errinf(qstate, "malloc failure, in safety belt");
					return error_response(qstate, id, 
						LDNS_RCODE_SERVFAIL);
				}
				return next_state(iq, INIT_REQUEST_2_STATE);
			}
			/* Note that the result of this will set a new
			 * DelegationPoint based on the result of priming. */
			if(!prime_root(qstate, iq, id, iq->qchase.qclass))
				return error_response(qstate, id, 
					LDNS_RCODE_REFUSED);

			/* priming creates and sends a subordinate query, with 
			 * this query as the parent. So further processing for 
			 * this event will stop until reactivated by the 
			 * results of priming. */
			return 0;
		}
		if(!iq->ratelimit_ok && qstate->prefetch_leeway)
			iq->ratelimit_ok = 1; /* allow prefetches, this keeps
			otherwise valid data in the cache */
		if(!iq->ratelimit_ok && infra_ratelimit_exceeded(
			qstate->env->infra_cache, iq->dp->name,
			iq->dp->namelen, *qstate->env->now)) {
			/* and increment the rate, so that the rate for time
			 * now will also exceed the rate, keeping cache fresh */
			(void)infra_ratelimit_inc(qstate->env->infra_cache,
				iq->dp->name, iq->dp->namelen,
				*qstate->env->now, &qstate->qinfo,
				qstate->reply);
			/* see if we are passed through with slip factor */
			if(qstate->env->cfg->ratelimit_factor != 0 &&
				ub_random_max(qstate->env->rnd,
				    qstate->env->cfg->ratelimit_factor) == 1) {
				iq->ratelimit_ok = 1;
				log_nametypeclass(VERB_ALGO, "ratelimit allowed through for "
					"delegation point", iq->dp->name,
					LDNS_RR_TYPE_NS, LDNS_RR_CLASS_IN);
			} else {
				lock_basic_lock(&ie->queries_ratelimit_lock);
				ie->num_queries_ratelimited++;
				lock_basic_unlock(&ie->queries_ratelimit_lock);
				log_nametypeclass(VERB_ALGO, "ratelimit exceeded with "
					"delegation point", iq->dp->name,
					LDNS_RR_TYPE_NS, LDNS_RR_CLASS_IN);
				qstate->was_ratelimited = 1;
				errinf(qstate, "query was ratelimited");
				errinf_dname(qstate, "for zone", iq->dp->name);
				return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
			}
		}

		/* see if this dp not useless.
		 * It is useless if:
		 *	o all NS items are required glue. 
		 *	  or the query is for NS item that is required glue.
		 *	o no addresses are provided.
		 *	o RD qflag is on.
		 * Instead, go up one level, and try to get even further
		 * If the root was useless, use safety belt information. 
		 * Only check cache returns, because replies for servers
		 * could be useless but lead to loops (bumping into the
		 * same server reply) if useless-checked.
		 */
		if(iter_dp_is_useless(&qstate->qinfo, qstate->query_flags, 
			iq->dp)) {
			struct delegpt* retdp = NULL;
			if(!can_have_last_resort(qstate->env, iq->dp->name, iq->dp->namelen, iq->qchase.qclass, &retdp)) {
				if(retdp) {
					verbose(VERB_QUERY, "cache has stub "
						"or fwd but no addresses, "
						"fallback to config");
					iq->dp = delegpt_copy(retdp,
						qstate->region);
					if(!iq->dp) {
						log_err("out of memory in "
							"stub/fwd fallback");
						errinf(qstate, "malloc failure, for fallback to config");
						return error_response(qstate,
						    id, LDNS_RCODE_SERVFAIL);
					}
					break;
				}
				verbose(VERB_ALGO, "useless dp "
					"but cannot go up, servfail");
				delegpt_log(VERB_ALGO, iq->dp);
				errinf(qstate, "no useful nameservers, "
					"and cannot go up");
				errinf_dname(qstate, "for zone", iq->dp->name);
				return error_response(qstate, id, 
					LDNS_RCODE_SERVFAIL);
			}
			if(dname_is_root(iq->dp->name)) {
				/* use safety belt */
				verbose(VERB_QUERY, "Cache has root NS but "
				"no addresses. Fallback to the safety belt.");
				iq->dp = hints_lookup_root(qstate->env->hints, 
					iq->qchase.qclass);
				/* note deleg_msg is from previous lookup,
				 * but RD is on, so it is not used */
				if(!iq->dp) {
					log_err("internal error: no hints dp");
					return error_response(qstate, id, 
						LDNS_RCODE_REFUSED);
				}
				iq->dp = delegpt_copy(iq->dp, qstate->region);
				if(!iq->dp) {
					log_err("out of memory in safety belt");
					errinf(qstate, "malloc failure, in safety belt, for root");
					return error_response(qstate, id, 
						LDNS_RCODE_SERVFAIL);
				}
				break;
			} else {
				verbose(VERB_ALGO, 
					"cache delegation was useless:");
				delegpt_log(VERB_ALGO, iq->dp);
				/* go up */
				delname = iq->dp->name;
				delnamelen = iq->dp->namelen;
				dname_remove_label(&delname, &delnamelen);
			}
		} else break;
	}

	verbose(VERB_ALGO, "cache delegation returns delegpt");
	delegpt_log(VERB_ALGO, iq->dp);

	/* Otherwise, set the current delegation point and move on to the 
	 * next state. */
	return next_state(iq, INIT_REQUEST_2_STATE);
}