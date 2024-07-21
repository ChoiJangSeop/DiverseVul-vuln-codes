dns_cache_lookup(struct module_env* env,
	uint8_t* qname, size_t qnamelen, uint16_t qtype, uint16_t qclass,
	uint16_t flags, struct regional* region, struct regional* scratch,
	int no_partial, uint8_t* dpname, size_t dpnamelen)
{
	struct lruhash_entry* e;
	struct query_info k;
	hashvalue_type h;
	time_t now = *env->now;
	struct ub_packed_rrset_key* rrset;

	/* lookup first, this has both NXdomains and ANSWER responses */
	k.qname = qname;
	k.qname_len = qnamelen;
	k.qtype = qtype;
	k.qclass = qclass;
	k.local_alias = NULL;
	h = query_info_hash(&k, flags);
	e = slabhash_lookup(env->msg_cache, h, &k, 0);
	if(e) {
		struct msgreply_entry* key = (struct msgreply_entry*)e->key;
		struct reply_info* data = (struct reply_info*)e->data;
		struct dns_msg* msg = tomsg(env, &key->key, data, region, now, 0,
			scratch);
		if(msg) {
			lock_rw_unlock(&e->lock);
			return msg;
		}
		/* could be msg==NULL; due to TTL or not all rrsets available */
		lock_rw_unlock(&e->lock);
	}

	/* see if a DNAME exists. Checked for first, to enforce that DNAMEs
	 * are more important, the CNAME is resynthesized and thus 
	 * consistent with the DNAME */
	if(!no_partial &&
		(rrset=find_closest_of_type(env, qname, qnamelen, qclass, now,
		LDNS_RR_TYPE_DNAME, 1))) {
		/* synthesize a DNAME+CNAME message based on this */
		enum sec_status sec_status = sec_status_unchecked;
		struct dns_msg* msg = synth_dname_msg(rrset, region, now, &k,
			&sec_status);
		if(msg) {
			struct ub_packed_rrset_key* cname_rrset;
			lock_rw_unlock(&rrset->entry.lock);
			/* now, after unlocking the DNAME rrset lock,
			 * check the sec_status, and see if we need to look
			 * up the CNAME record associated before it can
			 * be used */
			/* normally, only secure DNAMEs allowed from cache*/
			if(sec_status == sec_status_secure)
				return msg;
			/* but if we have a CNAME cached with this name, then we
			 * have previously already allowed this name to pass.
			 * the next cache lookup is going to fetch that CNAME itself,
			 * but it is better to have the (unsigned)DNAME + CNAME in
			 * that case */
			cname_rrset = rrset_cache_lookup(
				env->rrset_cache, qname, qnamelen,
				LDNS_RR_TYPE_CNAME, qclass, 0, now, 0);
			if(cname_rrset) {
				/* CNAME already synthesized by
				 * synth_dname_msg routine, so we can
				 * straight up return the msg */
				lock_rw_unlock(&cname_rrset->entry.lock);
				return msg;
			}
		} else {
			lock_rw_unlock(&rrset->entry.lock);
		}
	}

	/* see if we have CNAME for this domain,
	 * but not for DS records (which are part of the parent) */
	if(!no_partial && qtype != LDNS_RR_TYPE_DS &&
	   (rrset=rrset_cache_lookup(env->rrset_cache, qname, qnamelen, 
		LDNS_RR_TYPE_CNAME, qclass, 0, now, 0))) {
		uint8_t* wc = NULL;
		size_t wl;
		/* if the rrset is not a wildcard expansion, with wcname */
		/* because, if we return that CNAME rrset on its own, it is
		 * missing the NSEC or NSEC3 proof */
		if(!(val_rrset_wildcard(rrset, &wc, &wl) && wc != NULL)) {
			struct dns_msg* msg = rrset_msg(rrset, region, now, &k);
			if(msg) {
				lock_rw_unlock(&rrset->entry.lock);
				return msg;
			}
		}
		lock_rw_unlock(&rrset->entry.lock);
	}

	/* construct DS, DNSKEY messages from rrset cache. */
	if((qtype == LDNS_RR_TYPE_DS || qtype == LDNS_RR_TYPE_DNSKEY) &&
		(rrset=rrset_cache_lookup(env->rrset_cache, qname, qnamelen, 
		qtype, qclass, 0, now, 0))) {
		/* if the rrset is from the additional section, and the
		 * signatures have fallen off, then do not synthesize a msg
		 * instead, allow a full query for signed results to happen.
		 * Forego all rrset data from additional section, because
		 * some signatures may not be present and cause validation
		 * failure.
		 */
		struct packed_rrset_data *d = (struct packed_rrset_data*)
			rrset->entry.data;
		if(d->trust != rrset_trust_add_noAA && 
			d->trust != rrset_trust_add_AA && 
			(qtype == LDNS_RR_TYPE_DS || 
				(d->trust != rrset_trust_auth_noAA 
				&& d->trust != rrset_trust_auth_AA) )) {
			struct dns_msg* msg = rrset_msg(rrset, region, now, &k);
			if(msg) {
				lock_rw_unlock(&rrset->entry.lock);
				return msg;
			}
		}
		lock_rw_unlock(&rrset->entry.lock);
	}

	/* stop downwards cache search on NXDOMAIN.
	 * Empty nonterminals are NOERROR, so an NXDOMAIN for foo
	 * means bla.foo also does not exist.  The DNSSEC proofs are
	 * the same.  We search upwards for NXDOMAINs. */
	if(env->cfg->harden_below_nxdomain) {
		while(!dname_is_root(k.qname)) {
			if(dpname && dpnamelen
				&& !dname_subdomain_c(k.qname, dpname))
				break; /* no synth nxdomain above the stub */
			dname_remove_label(&k.qname, &k.qname_len);
			h = query_info_hash(&k, flags);
			e = slabhash_lookup(env->msg_cache, h, &k, 0);
			if(!e && k.qtype != LDNS_RR_TYPE_A &&
				env->cfg->qname_minimisation) {
				k.qtype = LDNS_RR_TYPE_A;
				h = query_info_hash(&k, flags);
				e = slabhash_lookup(env->msg_cache, h, &k, 0);
			}
			if(e) {
				struct reply_info* data = (struct reply_info*)e->data;
				struct dns_msg* msg;
				if(FLAGS_GET_RCODE(data->flags) == LDNS_RCODE_NXDOMAIN
					&& data->security == sec_status_secure
					&& (data->an_numrrsets == 0 ||
						ntohs(data->rrsets[0]->rk.type) != LDNS_RR_TYPE_CNAME)
					&& (msg=tomsg(env, &k, data, region, now, 0, scratch))) {
					lock_rw_unlock(&e->lock);
					msg->qinfo.qname=qname;
					msg->qinfo.qname_len=qnamelen;
					/* check that DNSSEC really works out */
					msg->rep->security = sec_status_unchecked;
					iter_scrub_nxdomain(msg);
					return msg;
				}
				lock_rw_unlock(&e->lock);
			}
			k.qtype = qtype;
		}
	}

	/* fill common RR types for ANY response to avoid requery */
	if(qtype == LDNS_RR_TYPE_ANY) {
		return fill_any(env, qname, qnamelen, qtype, qclass, region);
	}

	return NULL;
}