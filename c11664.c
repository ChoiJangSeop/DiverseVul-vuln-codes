processQueryResponse(struct module_qstate* qstate, struct iter_qstate* iq,
	struct iter_env* ie, int id)
{
	int dnsseclame = 0;
	enum response_type type;

	iq->num_current_queries--;

	if(!inplace_cb_query_response_call(qstate->env, qstate, iq->response))
		log_err("unable to call query_response callback");

	if(iq->response == NULL) {
		/* Don't increment qname when QNAME minimisation is enabled */
		if(qstate->env->cfg->qname_minimisation) {
			iq->minimisation_state = SKIP_MINIMISE_STATE;
		}
		iq->timeout_count++;
		iq->chase_to_rd = 0;
		iq->dnssec_lame_query = 0;
		verbose(VERB_ALGO, "query response was timeout");
		return next_state(iq, QUERYTARGETS_STATE);
	}
	iq->timeout_count = 0;
	type = response_type_from_server(
		(int)((iq->chase_flags&BIT_RD) || iq->chase_to_rd),
		iq->response, &iq->qinfo_out, iq->dp);
	iq->chase_to_rd = 0;
	if(type == RESPONSE_TYPE_REFERRAL && (iq->chase_flags&BIT_RD) &&
		!iq->auth_zone_response) {
		/* When forwarding (RD bit is set), we handle referrals 
		 * differently. No queries should be sent elsewhere */
		type = RESPONSE_TYPE_ANSWER;
	}
	if(!qstate->env->cfg->disable_dnssec_lame_check && iq->dnssec_expected 
                && !iq->dnssec_lame_query &&
		!(iq->chase_flags&BIT_RD) 
		&& iq->sent_count < DNSSEC_LAME_DETECT_COUNT
		&& type != RESPONSE_TYPE_LAME 
		&& type != RESPONSE_TYPE_REC_LAME 
		&& type != RESPONSE_TYPE_THROWAWAY 
		&& type != RESPONSE_TYPE_UNTYPED) {
		/* a possible answer, see if it is missing DNSSEC */
		/* but not when forwarding, so we dont mark fwder lame */
		if(!iter_msg_has_dnssec(iq->response)) {
			/* Mark this address as dnsseclame in this dp,
			 * because that will make serverselection disprefer
			 * it, but also, once it is the only final option,
			 * use dnssec-lame-bypass if it needs to query there.*/
			if(qstate->reply) {
				struct delegpt_addr* a = delegpt_find_addr(
					iq->dp, &qstate->reply->addr,
					qstate->reply->addrlen);
				if(a) a->dnsseclame = 1;
			}
			/* test the answer is from the zone we expected,
		 	 * otherwise, (due to parent,child on same server), we
		 	 * might mark the server,zone lame inappropriately */
			if(!iter_msg_from_zone(iq->response, iq->dp, type,
				iq->qchase.qclass))
				qstate->reply = NULL;
			type = RESPONSE_TYPE_LAME;
			dnsseclame = 1;
		}
	} else iq->dnssec_lame_query = 0;
	/* see if referral brings us close to the target */
	if(type == RESPONSE_TYPE_REFERRAL) {
		struct ub_packed_rrset_key* ns = find_NS(
			iq->response->rep, iq->response->rep->an_numrrsets,
			iq->response->rep->an_numrrsets 
			+ iq->response->rep->ns_numrrsets);
		if(!ns) ns = find_NS(iq->response->rep, 0, 
				iq->response->rep->an_numrrsets);
		if(!ns || !dname_strict_subdomain_c(ns->rk.dname, iq->dp->name) 
			|| !dname_subdomain_c(iq->qchase.qname, ns->rk.dname)){
			verbose(VERB_ALGO, "bad referral, throwaway");
			type = RESPONSE_TYPE_THROWAWAY;
		} else
			iter_scrub_ds(iq->response, ns, iq->dp->name);
	} else iter_scrub_ds(iq->response, NULL, NULL);
	if(type == RESPONSE_TYPE_THROWAWAY &&
		FLAGS_GET_RCODE(iq->response->rep->flags) == LDNS_RCODE_YXDOMAIN) {
		/* YXDOMAIN is a permanent error, no need to retry */
		type = RESPONSE_TYPE_ANSWER;
	}
	if(type == RESPONSE_TYPE_CNAME && iq->response->rep->an_numrrsets >= 1
		&& ntohs(iq->response->rep->rrsets[0]->rk.type) == LDNS_RR_TYPE_DNAME) {
		uint8_t* sname = NULL;
		size_t snamelen = 0;
		get_cname_target(iq->response->rep->rrsets[0], &sname,
			&snamelen);
		if(snamelen && dname_subdomain_c(sname, iq->response->rep->rrsets[0]->rk.dname)) {
			/* DNAME to a subdomain loop; do not recurse */
			type = RESPONSE_TYPE_ANSWER;
		}
	} else if(type == RESPONSE_TYPE_CNAME &&
		iq->qchase.qtype == LDNS_RR_TYPE_CNAME &&
		iq->minimisation_state == MINIMISE_STATE &&
		query_dname_compare(iq->qchase.qname, iq->qinfo_out.qname) == 0) {
		/* The minimised query for full QTYPE and hidden QTYPE can be
		 * classified as CNAME response type, even when the original
		 * QTYPE=CNAME. This should be treated as answer response type.
		 */
		type = RESPONSE_TYPE_ANSWER;
	}

	/* handle each of the type cases */
	if(type == RESPONSE_TYPE_ANSWER) {
		/* ANSWER type responses terminate the query algorithm, 
		 * so they sent on their */
		if(verbosity >= VERB_DETAIL) {
			verbose(VERB_DETAIL, "query response was %s",
				FLAGS_GET_RCODE(iq->response->rep->flags)
				==LDNS_RCODE_NXDOMAIN?"NXDOMAIN ANSWER":
				(iq->response->rep->an_numrrsets?"ANSWER":
				"nodata ANSWER"));
		}
		/* if qtype is DS, check we have the right level of answer,
		 * like grandchild answer but we need the middle, reject it */
		if(iq->qchase.qtype == LDNS_RR_TYPE_DS && !iq->dsns_point
			&& !(iq->chase_flags&BIT_RD)
			&& iter_ds_toolow(iq->response, iq->dp)
			&& iter_dp_cangodown(&iq->qchase, iq->dp)) {
			/* close down outstanding requests to be discarded */
			outbound_list_clear(&iq->outlist);
			iq->num_current_queries = 0;
			fptr_ok(fptr_whitelist_modenv_detach_subs(
				qstate->env->detach_subs));
			(*qstate->env->detach_subs)(qstate);
			iq->num_target_queries = 0;
			return processDSNSFind(qstate, iq, id);
		}
		if(!qstate->no_cache_store)
			iter_dns_store(qstate->env, &iq->response->qinfo,
				iq->response->rep,
				iq->qchase.qtype != iq->response->qinfo.qtype,
				qstate->prefetch_leeway,
				iq->dp&&iq->dp->has_parent_side_NS,
				qstate->region, qstate->query_flags);
		/* close down outstanding requests to be discarded */
		outbound_list_clear(&iq->outlist);
		iq->num_current_queries = 0;
		fptr_ok(fptr_whitelist_modenv_detach_subs(
			qstate->env->detach_subs));
		(*qstate->env->detach_subs)(qstate);
		iq->num_target_queries = 0;
		if(qstate->reply)
			sock_list_insert(&qstate->reply_origin, 
				&qstate->reply->addr, qstate->reply->addrlen, 
				qstate->region);
		if(iq->minimisation_state != DONOT_MINIMISE_STATE
			&& !(iq->chase_flags & BIT_RD)) {
			if(FLAGS_GET_RCODE(iq->response->rep->flags) != 
				LDNS_RCODE_NOERROR) {
				if(qstate->env->cfg->qname_minimisation_strict) {
					if(FLAGS_GET_RCODE(iq->response->rep->flags) ==
						LDNS_RCODE_NXDOMAIN) {
						iter_scrub_nxdomain(iq->response);
						return final_state(iq);
					}
					return error_response(qstate, id,
						LDNS_RCODE_SERVFAIL);
				}
				/* Best effort qname-minimisation. 
				 * Stop minimising and send full query when
				 * RCODE is not NOERROR. */
				iq->minimisation_state = DONOT_MINIMISE_STATE;
			}
			if(FLAGS_GET_RCODE(iq->response->rep->flags) ==
				LDNS_RCODE_NXDOMAIN) {
				/* Stop resolving when NXDOMAIN is DNSSEC
				 * signed. Based on assumption that nameservers
				 * serving signed zones do not return NXDOMAIN
				 * for empty-non-terminals. */
				if(iq->dnssec_expected)
					return final_state(iq);
				/* Make subrequest to validate intermediate
				 * NXDOMAIN if harden-below-nxdomain is
				 * enabled. */
				if(qstate->env->cfg->harden_below_nxdomain &&
					qstate->env->need_to_validate) {
					struct module_qstate* subq = NULL;
					log_query_info(VERB_QUERY,
						"schedule NXDOMAIN validation:",
						&iq->response->qinfo);
					if(!generate_sub_request(
						iq->response->qinfo.qname,
						iq->response->qinfo.qname_len,
						iq->response->qinfo.qtype,
						iq->response->qinfo.qclass,
						qstate, id, iq,
						INIT_REQUEST_STATE,
						FINISHED_STATE, &subq, 1, 1))
						verbose(VERB_ALGO,
						"could not validate NXDOMAIN "
						"response");
				}
			}
			return next_state(iq, QUERYTARGETS_STATE);
		}
		return final_state(iq);
	} else if(type == RESPONSE_TYPE_REFERRAL) {
		/* REFERRAL type responses get a reset of the 
		 * delegation point, and back to the QUERYTARGETS_STATE. */
		verbose(VERB_DETAIL, "query response was REFERRAL");

		/* if hardened, only store referral if we asked for it */
		if(!qstate->no_cache_store &&
		(!qstate->env->cfg->harden_referral_path ||
		    (  qstate->qinfo.qtype == LDNS_RR_TYPE_NS 
			&& (qstate->query_flags&BIT_RD) 
			&& !(qstate->query_flags&BIT_CD)
			   /* we know that all other NS rrsets are scrubbed
			    * away, thus on referral only one is left.
			    * see if that equals the query name... */
			&& ( /* auth section, but sometimes in answer section*/
			  reply_find_rrset_section_ns(iq->response->rep,
				iq->qchase.qname, iq->qchase.qname_len,
				LDNS_RR_TYPE_NS, iq->qchase.qclass)
			  || reply_find_rrset_section_an(iq->response->rep,
				iq->qchase.qname, iq->qchase.qname_len,
				LDNS_RR_TYPE_NS, iq->qchase.qclass)
			  )
		    ))) {
			/* Store the referral under the current query */
			/* no prefetch-leeway, since its not the answer */
			iter_dns_store(qstate->env, &iq->response->qinfo,
				iq->response->rep, 1, 0, 0, NULL, 0);
			if(iq->store_parent_NS)
				iter_store_parentside_NS(qstate->env, 
					iq->response->rep);
			if(qstate->env->neg_cache)
				val_neg_addreferral(qstate->env->neg_cache, 
					iq->response->rep, iq->dp->name);
		}
		/* store parent-side-in-zone-glue, if directly queried for */
		if(!qstate->no_cache_store && iq->query_for_pside_glue
			&& !iq->pside_glue) {
				iq->pside_glue = reply_find_rrset(iq->response->rep, 
					iq->qchase.qname, iq->qchase.qname_len, 
					iq->qchase.qtype, iq->qchase.qclass);
				if(iq->pside_glue) {
					log_rrset_key(VERB_ALGO, "found parent-side "
						"glue", iq->pside_glue);
					iter_store_parentside_rrset(qstate->env,
						iq->pside_glue);
				}
		}

		/* Reset the event state, setting the current delegation 
		 * point to the referral. */
		iq->deleg_msg = iq->response;
		iq->dp = delegpt_from_message(iq->response, qstate->region);
		if (qstate->env->cfg->qname_minimisation)
			iq->minimisation_state = INIT_MINIMISE_STATE;
		if(!iq->dp) {
			errinf(qstate, "malloc failure, for delegation point");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		if(!cache_fill_missing(qstate->env, iq->qchase.qclass, 
			qstate->region, iq->dp)) {
			errinf(qstate, "malloc failure, copy extra info into delegation point");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		if(iq->store_parent_NS && query_dname_compare(iq->dp->name,
			iq->store_parent_NS->name) == 0)
			iter_merge_retry_counts(iq->dp, iq->store_parent_NS,
				ie->outbound_msg_retry);
		delegpt_log(VERB_ALGO, iq->dp);
		/* Count this as a referral. */
		iq->referral_count++;
		iq->sent_count = 0;
		iq->dp_target_count = 0;
		/* see if the next dp is a trust anchor, or a DS was sent
		 * along, indicating dnssec is expected for next zone */
		iq->dnssec_expected = iter_indicates_dnssec(qstate->env, 
			iq->dp, iq->response, iq->qchase.qclass);
		/* if dnssec, validating then also fetch the key for the DS */
		if(iq->dnssec_expected && qstate->env->cfg->prefetch_key &&
			!(qstate->query_flags&BIT_CD))
			generate_dnskey_prefetch(qstate, iq, id);

		/* spawn off NS and addr to auth servers for the NS we just
		 * got in the referral. This gets authoritative answer
		 * (answer section trust level) rrset. 
		 * right after, we detach the subs, answer goes to cache. */
		if(qstate->env->cfg->harden_referral_path)
			generate_ns_check(qstate, iq, id);

		/* stop current outstanding queries. 
		 * FIXME: should the outstanding queries be waited for and
		 * handled? Say by a subquery that inherits the outbound_entry.
		 */
		outbound_list_clear(&iq->outlist);
		iq->num_current_queries = 0;
		fptr_ok(fptr_whitelist_modenv_detach_subs(
			qstate->env->detach_subs));
		(*qstate->env->detach_subs)(qstate);
		iq->num_target_queries = 0;
		iq->response = NULL;
		iq->fail_reply = NULL;
		verbose(VERB_ALGO, "cleared outbound list for next round");
		return next_state(iq, QUERYTARGETS_STATE);
	} else if(type == RESPONSE_TYPE_CNAME) {
		uint8_t* sname = NULL;
		size_t snamelen = 0;
		/* CNAME type responses get a query restart (i.e., get a 
		 * reset of the query state and go back to INIT_REQUEST_STATE).
		 */
		verbose(VERB_DETAIL, "query response was CNAME");
		if(verbosity >= VERB_ALGO)
			log_dns_msg("cname msg", &iq->response->qinfo, 
				iq->response->rep);
		/* if qtype is DS, check we have the right level of answer,
		 * like grandchild answer but we need the middle, reject it */
		if(iq->qchase.qtype == LDNS_RR_TYPE_DS && !iq->dsns_point
			&& !(iq->chase_flags&BIT_RD)
			&& iter_ds_toolow(iq->response, iq->dp)
			&& iter_dp_cangodown(&iq->qchase, iq->dp)) {
			outbound_list_clear(&iq->outlist);
			iq->num_current_queries = 0;
			fptr_ok(fptr_whitelist_modenv_detach_subs(
				qstate->env->detach_subs));
			(*qstate->env->detach_subs)(qstate);
			iq->num_target_queries = 0;
			return processDSNSFind(qstate, iq, id);
		}
		/* Process the CNAME response. */
		if(!handle_cname_response(qstate, iq, iq->response, 
			&sname, &snamelen)) {
			errinf(qstate, "malloc failure, CNAME info");
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		/* cache the CNAME response under the current query */
		/* NOTE : set referral=1, so that rrsets get stored but not 
		 * the partial query answer (CNAME only). */
		/* prefetchleeway applied because this updates answer parts */
		if(!qstate->no_cache_store)
			iter_dns_store(qstate->env, &iq->response->qinfo,
				iq->response->rep, 1, qstate->prefetch_leeway,
				iq->dp&&iq->dp->has_parent_side_NS, NULL,
				qstate->query_flags);
		/* set the current request's qname to the new value. */
		iq->qchase.qname = sname;
		iq->qchase.qname_len = snamelen;
		if(qstate->env->auth_zones) {
			/* apply rpz qname triggers after cname */
			struct dns_msg* forged_response =
				rpz_callback_from_iterator_cname(qstate, iq);
			while(forged_response && reply_find_rrset_section_an(
				forged_response->rep, iq->qchase.qname,
				iq->qchase.qname_len, LDNS_RR_TYPE_CNAME,
				iq->qchase.qclass)) {
				/* another cname to follow */
				if(!handle_cname_response(qstate, iq, forged_response,
					&sname, &snamelen)) {
					errinf(qstate, "malloc failure, CNAME info");
					return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
				}
				iq->qchase.qname = sname;
				iq->qchase.qname_len = snamelen;
				forged_response =
					rpz_callback_from_iterator_cname(qstate, iq);
			}
			if(forged_response != NULL) {
				qstate->ext_state[id] = module_finished;
				qstate->return_rcode = LDNS_RCODE_NOERROR;
				qstate->return_msg = forged_response;
				iq->response = forged_response;
				next_state(iq, FINISHED_STATE);
				if(!iter_prepend(iq, qstate->return_msg, qstate->region)) {
					log_err("rpz: after cname, prepend rrsets: out of memory");
					return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
				}
				qstate->return_msg->qinfo = qstate->qinfo;
				return 0;
			}
		}
		/* Clear the query state, since this is a query restart. */
		iq->deleg_msg = NULL;
		iq->dp = NULL;
		iq->dsns_point = NULL;
		iq->auth_zone_response = 0;
		iq->sent_count = 0;
		iq->dp_target_count = 0;
		if(iq->minimisation_state != MINIMISE_STATE)
			/* Only count as query restart when it is not an extra
			 * query as result of qname minimisation. */
			iq->query_restart_count++;
		if(qstate->env->cfg->qname_minimisation)
			iq->minimisation_state = INIT_MINIMISE_STATE;

		/* stop current outstanding queries. 
		 * FIXME: should the outstanding queries be waited for and
		 * handled? Say by a subquery that inherits the outbound_entry.
		 */
		outbound_list_clear(&iq->outlist);
		iq->num_current_queries = 0;
		fptr_ok(fptr_whitelist_modenv_detach_subs(
			qstate->env->detach_subs));
		(*qstate->env->detach_subs)(qstate);
		iq->num_target_queries = 0;
		if(qstate->reply)
			sock_list_insert(&qstate->reply_origin, 
				&qstate->reply->addr, qstate->reply->addrlen, 
				qstate->region);
		verbose(VERB_ALGO, "cleared outbound list for query restart");
		/* go to INIT_REQUEST_STATE for new qname. */
		return next_state(iq, INIT_REQUEST_STATE);
	} else if(type == RESPONSE_TYPE_LAME) {
		/* Cache the LAMEness. */
		verbose(VERB_DETAIL, "query response was %sLAME",
			dnsseclame?"DNSSEC ":"");
		if(!dname_subdomain_c(iq->qchase.qname, iq->dp->name)) {
			log_err("mark lame: mismatch in qname and dpname");
			/* throwaway this reply below */
		} else if(qstate->reply) {
			/* need addr for lameness cache, but we may have
			 * gotten this from cache, so test to be sure */
			if(!infra_set_lame(qstate->env->infra_cache, 
				&qstate->reply->addr, qstate->reply->addrlen, 
				iq->dp->name, iq->dp->namelen, 
				*qstate->env->now, dnsseclame, 0,
				iq->qchase.qtype))
				log_err("mark host lame: out of memory");
		}
	} else if(type == RESPONSE_TYPE_REC_LAME) {
		/* Cache the LAMEness. */
		verbose(VERB_DETAIL, "query response REC_LAME: "
			"recursive but not authoritative server");
		if(!dname_subdomain_c(iq->qchase.qname, iq->dp->name)) {
			log_err("mark rec_lame: mismatch in qname and dpname");
			/* throwaway this reply below */
		} else if(qstate->reply) {
			/* need addr for lameness cache, but we may have
			 * gotten this from cache, so test to be sure */
			verbose(VERB_DETAIL, "mark as REC_LAME");
			if(!infra_set_lame(qstate->env->infra_cache, 
				&qstate->reply->addr, qstate->reply->addrlen, 
				iq->dp->name, iq->dp->namelen, 
				*qstate->env->now, 0, 1, iq->qchase.qtype))
				log_err("mark host lame: out of memory");
		} 
	} else if(type == RESPONSE_TYPE_THROWAWAY) {
		/* LAME and THROWAWAY responses are handled the same way. 
		 * In this case, the event is just sent directly back to 
		 * the QUERYTARGETS_STATE without resetting anything, 
		 * because, clearly, the next target must be tried. */
		verbose(VERB_DETAIL, "query response was THROWAWAY");
	} else {
		log_warn("A query response came back with an unknown type: %d",
			(int)type);
	}

	/* LAME, THROWAWAY and "unknown" all end up here.
	 * Recycle to the QUERYTARGETS state to hopefully try a 
	 * different target. */
	if (qstate->env->cfg->qname_minimisation &&
		!qstate->env->cfg->qname_minimisation_strict)
		iq->minimisation_state = DONOT_MINIMISE_STATE;
	if(iq->auth_zone_response) {
		/* can we fallback? */
		iq->auth_zone_response = 0;
		if(!auth_zones_can_fallback(qstate->env->auth_zones,
			iq->dp->name, iq->dp->namelen, qstate->qinfo.qclass)) {
			verbose(VERB_ALGO, "auth zone response bad, and no"
				" fallback possible, servfail");
			errinf_dname(qstate, "response is bad, no fallback, "
				"for auth zone", iq->dp->name);
			return error_response(qstate, id, LDNS_RCODE_SERVFAIL);
		}
		verbose(VERB_ALGO, "auth zone response was bad, "
			"fallback enabled");
		iq->auth_zone_avoid = 1;
		if(iq->dp->auth_dp) {
			/* we are using a dp for the auth zone, with no
			 * nameservers, get one first */
			iq->dp = NULL;
			return next_state(iq, INIT_REQUEST_STATE);
		}
	}
	return next_state(iq, QUERYTARGETS_STATE);
}