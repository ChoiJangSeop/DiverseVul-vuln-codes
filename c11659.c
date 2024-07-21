int print_deleg_lookup(RES* ssl, struct worker* worker, uint8_t* nm,
	size_t nmlen, int ATTR_UNUSED(nmlabs))
{
	/* deep links into the iterator module */
	struct delegpt* dp;
	struct dns_msg* msg;
	struct regional* region = worker->scratchpad;
	char b[260];
	struct query_info qinfo;
	struct iter_hints_stub* stub;
	regional_free_all(region);
	qinfo.qname = nm;
	qinfo.qname_len = nmlen;
	qinfo.qtype = LDNS_RR_TYPE_A;
	qinfo.qclass = LDNS_RR_CLASS_IN;
	qinfo.local_alias = NULL;

	dname_str(nm, b);
	if(!ssl_printf(ssl, "The following name servers are used for lookup "
		"of %s\n", b)) 
		return 0;
	
	dp = forwards_lookup(worker->env.fwds, nm, qinfo.qclass);
	if(dp) {
		if(!ssl_printf(ssl, "forwarding request:\n"))
			return 0;
		print_dp_main(ssl, dp, NULL);
		print_dp_details(ssl, worker, dp);
		return 1;
	}
	
	while(1) {
		dp = dns_cache_find_delegation(&worker->env, nm, nmlen, 
			qinfo.qtype, qinfo.qclass, region, &msg, 
			*worker->env.now);
		if(!dp) {
			return ssl_printf(ssl, "no delegation from "
				"cache; goes to configured roots\n");
		}
		/* go up? */
		if(iter_dp_is_useless(&qinfo, BIT_RD, dp,
			(worker->env.cfg->do_ip4 && worker->back->num_ip4 != 0),
			(worker->env.cfg->do_ip6 && worker->back->num_ip6 != 0))) {
			print_dp_main(ssl, dp, msg);
			print_dp_details(ssl, worker, dp);
			if(!ssl_printf(ssl, "cache delegation was "
				"useless (no IP addresses)\n"))
				return 0;
			if(dname_is_root(nm)) {
				/* goes to root config */
				return ssl_printf(ssl, "no delegation from "
					"cache; goes to configured roots\n");
			} else {
				/* useless, goes up */
				nm = dp->name;
				nmlen = dp->namelen;
				dname_remove_label(&nm, &nmlen);
				dname_str(nm, b);
				if(!ssl_printf(ssl, "going up, lookup %s\n", b))
					return 0;
				continue;
			}
		} 
		stub = hints_lookup_stub(worker->env.hints, nm, qinfo.qclass,
			dp);
		if(stub) {
			if(stub->noprime) {
				if(!ssl_printf(ssl, "The noprime stub servers "
					"are used:\n"))
					return 0;
			} else {
				if(!ssl_printf(ssl, "The stub is primed "
						"with servers:\n"))
					return 0;
			}
			print_dp_main(ssl, stub->dp, NULL);
			print_dp_details(ssl, worker, stub->dp);
		} else {
			print_dp_main(ssl, dp, msg);
			print_dp_details(ssl, worker, dp);
		}
		break;
	}

	return 1;
}