ns_xfr_start(ns_client_t *client, dns_rdatatype_t reqtype) {
	isc_result_t result;
	dns_name_t *question_name;
	dns_rdataset_t *question_rdataset;
	dns_zone_t *zone = NULL, *raw = NULL, *mayberaw;
	dns_db_t *db = NULL;
	dns_dbversion_t *ver = NULL;
	dns_rdataclass_t question_class;
	rrstream_t *soa_stream = NULL;
	rrstream_t *data_stream = NULL;
	rrstream_t *stream = NULL;
	dns_difftuple_t *current_soa_tuple = NULL;
	dns_name_t *soa_name;
	dns_rdataset_t *soa_rdataset;
	dns_rdata_t soa_rdata = DNS_RDATA_INIT;
	bool have_soa = false;
	const char *mnemonic = NULL;
	isc_mem_t *mctx = client->mctx;
	dns_message_t *request = client->message;
	xfrout_ctx_t *xfr = NULL;
	isc_quota_t *quota = NULL;
	dns_transfer_format_t format = client->view->transfer_format;
	isc_netaddr_t na;
	dns_peer_t *peer = NULL;
	isc_buffer_t *tsigbuf = NULL;
	char *journalfile;
	char msg[NS_CLIENT_ACLMSGSIZE("zone transfer")];
	char keyname[DNS_NAME_FORMATSIZE];
	bool is_poll = false;
	bool is_dlz = false;
	bool is_ixfr = false;
	uint32_t begin_serial = 0, current_serial;

	switch (reqtype) {
	case dns_rdatatype_axfr:
		mnemonic = "AXFR";
		break;
	case dns_rdatatype_ixfr:
		mnemonic = "IXFR";
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	ns_client_log(client,
		      DNS_LOGCATEGORY_XFER_OUT, NS_LOGMODULE_XFER_OUT,
		      ISC_LOG_DEBUG(6), "%s request", mnemonic);
	/*
	 * Apply quota.
	 */
	result = isc_quota_attach(&ns_g_server->xfroutquota, &quota);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(XFROUT_COMMON_LOGARGS, ISC_LOG_WARNING,
			      "%s request denied: %s", mnemonic,
			      isc_result_totext(result));
		goto failure;
	}

	/*
	 * Interpret the question section.
	 */
	result = dns_message_firstname(request, DNS_SECTION_QUESTION);
	INSIST(result == ISC_R_SUCCESS);

	/*
	 * The question section must contain exactly one question, and
	 * it must be for AXFR/IXFR as appropriate.
	 */
	question_name = NULL;
	dns_message_currentname(request, DNS_SECTION_QUESTION, &question_name);
	question_rdataset = ISC_LIST_HEAD(question_name->list);
	question_class = question_rdataset->rdclass;
	INSIST(question_rdataset->type == reqtype);
	if (ISC_LIST_NEXT(question_rdataset, link) != NULL)
		FAILC(DNS_R_FORMERR, "multiple questions");
	result = dns_message_nextname(request, DNS_SECTION_QUESTION);
	if (result != ISC_R_NOMORE)
		FAILC(DNS_R_FORMERR, "multiple questions");

	result = dns_zt_find(client->view->zonetable, question_name, 0, NULL,
			     &zone);

	if (result != ISC_R_SUCCESS) {
		/*
		 * Normal zone table does not have a match.
		 * Try the DLZ database
		 */
		// Temporary: only searching the first DLZ database
		if (! ISC_LIST_EMPTY(client->view->dlz_searched)) {
			result = dns_dlzallowzonexfr(client->view,
						     question_name,
						     &client->peeraddr,
						     &db);

			if (result == ISC_R_NOPERM) {
				char _buf1[DNS_NAME_FORMATSIZE];
				char _buf2[DNS_RDATACLASS_FORMATSIZE];

				result = DNS_R_REFUSED;
				dns_name_format(question_name, _buf1,
						sizeof(_buf1));
				dns_rdataclass_format(question_class,
						      _buf2, sizeof(_buf2));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_XFER_OUT,
					      ISC_LOG_ERROR,
					      "zone transfer '%s/%s' denied",
					      _buf1, _buf2);
				goto failure;
			}
			if (result != ISC_R_SUCCESS)
				FAILQ(DNS_R_NOTAUTH, "non-authoritative zone",
				      question_name, question_class);
			is_dlz = true;
		} else {
			/*
			 * not DLZ and not in normal zone table, we are
			 * not authoritative
			 */
			FAILQ(DNS_R_NOTAUTH, "non-authoritative zone",
			      question_name, question_class);
		}
	} else {
		/* zone table has a match */
		switch(dns_zone_gettype(zone)) {
			/* Master and slave zones are OK for transfer. */
			case dns_zone_master:
			case dns_zone_slave:
			case dns_zone_dlz:
				break;
			default:
				FAILQ(DNS_R_NOTAUTH, "non-authoritative zone",
				      question_name, question_class);
			}
		CHECK(dns_zone_getdb(zone, &db));
		dns_db_currentversion(db, &ver);
	}

	xfrout_log1(client, question_name, question_class, ISC_LOG_DEBUG(6),
		    "%s question section OK", mnemonic);

	/*
	 * Check the authority section.  Look for a SOA record with
	 * the same name and class as the question.
	 */
	for (result = dns_message_firstname(request, DNS_SECTION_AUTHORITY);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(request, DNS_SECTION_AUTHORITY))
	{
		soa_name = NULL;
		dns_message_currentname(request, DNS_SECTION_AUTHORITY,
					&soa_name);

		/*
		 * Ignore data whose owner name is not the zone apex.
		 */
		if (! dns_name_equal(soa_name, question_name))
			continue;

		for (soa_rdataset = ISC_LIST_HEAD(soa_name->list);
		     soa_rdataset != NULL;
		     soa_rdataset = ISC_LIST_NEXT(soa_rdataset, link))
		{
			/*
			 * Ignore non-SOA data.
			 */
			if (soa_rdataset->type != dns_rdatatype_soa)
				continue;
			if (soa_rdataset->rdclass != question_class)
				continue;

			CHECK(dns_rdataset_first(soa_rdataset));
			dns_rdataset_current(soa_rdataset, &soa_rdata);
			result = dns_rdataset_next(soa_rdataset);
			if (result == ISC_R_SUCCESS)
				FAILC(DNS_R_FORMERR,
				      "IXFR authority section "
				      "has multiple SOAs");
			have_soa = true;
			goto got_soa;
		}
	}
 got_soa:
	if (result != ISC_R_NOMORE)
		CHECK(result);

	xfrout_log1(client, question_name, question_class, ISC_LOG_DEBUG(6),
		    "%s authority section OK", mnemonic);

	/*
	 * If not a DLZ zone, decide whether to allow this transfer.
	 */
	if (!is_dlz) {
		ns_client_aclmsg("zone transfer", question_name, reqtype,
				 client->view->rdclass, msg, sizeof(msg));
		CHECK(ns_client_checkacl(client, NULL, msg,
					 dns_zone_getxfracl(zone),
					 true, ISC_LOG_ERROR));
	}

	/*
	 * AXFR over UDP is not possible.
	 */
	if (reqtype == dns_rdatatype_axfr &&
	    (client->attributes & NS_CLIENTATTR_TCP) == 0)
		FAILC(DNS_R_FORMERR, "attempted AXFR over UDP");

	/*
	 * Look up the requesting server in the peer table.
	 */
	isc_netaddr_fromsockaddr(&na, &client->peeraddr);
	(void)dns_peerlist_peerbyaddr(client->view->peers, &na, &peer);

	/*
	 * Decide on the transfer format (one-answer or many-answers).
	 */
	if (peer != NULL)
		(void)dns_peer_gettransferformat(peer, &format);

	/*
	 * Get a dynamically allocated copy of the current SOA.
	 */
	if (is_dlz)
		dns_db_currentversion(db, &ver);

	CHECK(dns_db_createsoatuple(db, ver, mctx, DNS_DIFFOP_EXISTS,
				    &current_soa_tuple));

	current_serial = dns_soa_getserial(&current_soa_tuple->rdata);
	if (reqtype == dns_rdatatype_ixfr) {
		bool provide_ixfr;

		/*
		 * Outgoing IXFR may have been disabled for this peer
		 * or globally.
		 */
		provide_ixfr = client->view->provideixfr;
		if (peer != NULL)
			(void) dns_peer_getprovideixfr(peer, &provide_ixfr);
		if (provide_ixfr == false)
			goto axfr_fallback;

		if (! have_soa)
			FAILC(DNS_R_FORMERR,
			      "IXFR request missing SOA");

		begin_serial = dns_soa_getserial(&soa_rdata);

		/*
		 * RFC1995 says "If an IXFR query with the same or
		 * newer version number than that of the server
		 * is received, it is replied to with a single SOA
		 * record of the server's current version, just as
		 * in AXFR".  The claim about AXFR is incorrect,
		 * but other than that, we do as the RFC says.
		 *
		 * Sending a single SOA record is also how we refuse
		 * IXFR over UDP (currently, we always do).
		 */
		if (DNS_SERIAL_GE(begin_serial, current_serial) ||
		    (client->attributes & NS_CLIENTATTR_TCP) == 0)
		{
			CHECK(soa_rrstream_create(mctx, db, ver, &stream));
			is_poll = true;
			goto have_stream;
		}
		journalfile = is_dlz ? NULL : dns_zone_getjournal(zone);
		if (journalfile != NULL)
			result = ixfr_rrstream_create(mctx,
						      journalfile,
						      begin_serial,
						      current_serial,
						      &data_stream);
		else
			result = ISC_R_NOTFOUND;
		if (result == ISC_R_NOTFOUND ||
		    result == ISC_R_RANGE) {
			xfrout_log1(client, question_name, question_class,
				    ISC_LOG_DEBUG(4),
				    "IXFR version not in journal, "
				    "falling back to AXFR");
			mnemonic = "AXFR-style IXFR";
			goto axfr_fallback;
		}
		CHECK(result);
		is_ixfr = true;
	} else {
	axfr_fallback:
		CHECK(axfr_rrstream_create(mctx, db, ver, &data_stream));
	}

	/*
	 * Bracket the data stream with SOAs.
	 */
	CHECK(soa_rrstream_create(mctx, db, ver, &soa_stream));
	CHECK(compound_rrstream_create(mctx, &soa_stream, &data_stream,
				       &stream));
	soa_stream = NULL;
	data_stream = NULL;

 have_stream:
	CHECK(dns_message_getquerytsig(request, mctx, &tsigbuf));
	/*
	 * Create the xfrout context object.  This transfers the ownership
	 * of "stream", "db", "ver", and "quota" to the xfrout context object.
	 */



	if (is_dlz)
		CHECK(xfrout_ctx_create(mctx, client, request->id,
					question_name, reqtype, question_class,
					zone, db, ver, quota, stream,
					dns_message_gettsigkey(request),
					tsigbuf,
					request->verified_sig,
					3600,
					3600,
					(format == dns_many_answers) ?
					true : false,
					&xfr));
	else
		CHECK(xfrout_ctx_create(mctx, client, request->id,
					question_name, reqtype, question_class,
					zone, db, ver, quota, stream,
					dns_message_gettsigkey(request),
					tsigbuf,
					request->verified_sig,
					dns_zone_getmaxxfrout(zone),
					dns_zone_getidleout(zone),
					(format == dns_many_answers) ?
					true : false,
					&xfr));

	xfr->mnemonic = mnemonic;
	stream = NULL;
	quota = NULL;

	CHECK(xfr->stream->methods->first(xfr->stream));

	if (xfr->tsigkey != NULL)
		dns_name_format(&xfr->tsigkey->name, keyname, sizeof(keyname));
	else
		keyname[0] = '\0';
	if (is_poll)
		xfrout_log1(client, question_name, question_class,
			    ISC_LOG_DEBUG(1), "IXFR poll up to date%s%s",
			    (xfr->tsigkey != NULL) ? ": TSIG " : "", keyname);
	else if (is_ixfr)
		xfrout_log1(client, question_name, question_class,
			    ISC_LOG_INFO, "%s started%s%s (serial %u -> %u)",
			    mnemonic, (xfr->tsigkey != NULL) ? ": TSIG " : "",
			    keyname, begin_serial, current_serial);
	else
		xfrout_log1(client, question_name, question_class,
			    ISC_LOG_INFO, "%s started%s%s (serial %u)",
			    mnemonic, (xfr->tsigkey != NULL) ? ": TSIG " : "",
			    keyname, current_serial);


	if (zone != NULL) {
		dns_zone_getraw(zone, &raw);
		mayberaw = (raw != NULL) ? raw : zone;
		if ((client->attributes & NS_CLIENTATTR_WANTEXPIRE) != 0 &&
		    dns_zone_gettype(mayberaw) == dns_zone_slave) {
			isc_time_t expiretime;
			uint32_t secs;
			dns_zone_getexpiretime(zone, &expiretime);
			secs = isc_time_seconds(&expiretime);
			if (secs >= client->now && result == ISC_R_SUCCESS) {
				client->attributes |= NS_CLIENTATTR_HAVEEXPIRE;
				client->expire = secs - client->now;
			}
		}
		if (raw != NULL)
			dns_zone_detach(&raw);
	}

	/*
	 * Hand the context over to sendstream().  Set xfr to NULL;
	 * sendstream() is responsible for either passing the
	 * context on to a later event handler or destroying it.
	 */
	sendstream(xfr);
	xfr = NULL;

	result = ISC_R_SUCCESS;

 failure:
	if (result == DNS_R_REFUSED)
		inc_stats(zone, dns_nsstatscounter_xfrrej);
	if (quota != NULL)
		isc_quota_detach(&quota);
	if (current_soa_tuple != NULL)
		dns_difftuple_free(&current_soa_tuple);
	if (stream != NULL)
		stream->methods->destroy(&stream);
	if (soa_stream != NULL)
		soa_stream->methods->destroy(&soa_stream);
	if (data_stream != NULL)
		data_stream->methods->destroy(&data_stream);
	if (ver != NULL)
		dns_db_closeversion(db, &ver, false);
	if (db != NULL)
		dns_db_detach(&db);
	if (zone != NULL)
		dns_zone_detach(&zone);
	/* XXX kludge */
	if (xfr != NULL) {
		xfrout_fail(xfr, result, "setting up zone transfer");
	} else if (result != ISC_R_SUCCESS) {
		ns_client_log(client, DNS_LOGCATEGORY_XFER_OUT,
			      NS_LOGMODULE_XFER_OUT,
			      ISC_LOG_DEBUG(3), "zone transfer setup failed");
		ns_client_error(client, result);
	}
}