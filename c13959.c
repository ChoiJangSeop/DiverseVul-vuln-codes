static NTSTATUS ldapsrv_SearchRequest(struct ldapsrv_call *call)
{
	struct ldap_SearchRequest *req = &call->request->r.SearchRequest;
	struct ldap_Result *done;
	struct ldapsrv_reply *done_r;
	TALLOC_CTX *local_ctx;
	struct ldapsrv_context *callback_ctx = NULL;
	struct ldb_context *samdb = talloc_get_type(call->conn->ldb, struct ldb_context);
	struct ldb_dn *basedn;
	struct ldb_request *lreq;
	struct ldb_control *search_control;
	struct ldb_search_options_control *search_options;
	struct ldb_control *extended_dn_control;
	struct ldb_extended_dn_control *extended_dn_decoded = NULL;
	struct ldb_control *notification_control = NULL;
	enum ldb_scope scope = LDB_SCOPE_DEFAULT;
	const char **attrs = NULL;
	const char *scope_str, *errstr = NULL;
	int result = -1;
	int ldb_ret = -1;
	unsigned int i;
	int extended_type = 1;

	DEBUG(10, ("SearchRequest"));
	DEBUGADD(10, (" basedn: %s", req->basedn));
	DEBUGADD(10, (" filter: %s\n", ldb_filter_from_tree(call, req->tree)));

	local_ctx = talloc_new(call);
	NT_STATUS_HAVE_NO_MEMORY(local_ctx);

	basedn = ldb_dn_new(local_ctx, samdb, req->basedn);
	NT_STATUS_HAVE_NO_MEMORY(basedn);

	DEBUG(10, ("SearchRequest: basedn: [%s]\n", req->basedn));
	DEBUG(10, ("SearchRequest: filter: [%s]\n", ldb_filter_from_tree(call, req->tree)));

	switch (req->scope) {
		case LDAP_SEARCH_SCOPE_BASE:
			scope_str = "BASE";
			scope = LDB_SCOPE_BASE;
			break;
		case LDAP_SEARCH_SCOPE_SINGLE:
			scope_str = "ONE";
			scope = LDB_SCOPE_ONELEVEL;
			break;
		case LDAP_SEARCH_SCOPE_SUB:
			scope_str = "SUB";
			scope = LDB_SCOPE_SUBTREE;
			break;
	        default:
			result = LDAP_PROTOCOL_ERROR;
			map_ldb_error(local_ctx, LDB_ERR_PROTOCOL_ERROR, NULL,
				&errstr);
			errstr = talloc_asprintf(local_ctx,
				"%s. Invalid scope", errstr);
			goto reply;
	}
	DEBUG(10,("SearchRequest: scope: [%s]\n", scope_str));

	if (req->num_attributes >= 1) {
		attrs = talloc_array(local_ctx, const char *, req->num_attributes+1);
		NT_STATUS_HAVE_NO_MEMORY(attrs);

		for (i=0; i < req->num_attributes; i++) {
			DEBUG(10,("SearchRequest: attrs: [%s]\n",req->attributes[i]));
			attrs[i] = req->attributes[i];
		}
		attrs[i] = NULL;
	}

	DEBUG(5,("ldb_request %s dn=%s filter=%s\n", 
		 scope_str, req->basedn, ldb_filter_from_tree(call, req->tree)));

	callback_ctx = talloc_zero(local_ctx, struct ldapsrv_context);
	NT_STATUS_HAVE_NO_MEMORY(callback_ctx);
	callback_ctx->call = call;
	callback_ctx->extended_type = extended_type;
	callback_ctx->attributesonly = req->attributesonly;

	ldb_ret = ldb_build_search_req_ex(&lreq, samdb, local_ctx,
					  basedn, scope,
					  req->tree, attrs,
					  call->request->controls,
					  callback_ctx,
					  ldap_server_search_callback,
					  NULL);

	if (ldb_ret != LDB_SUCCESS) {
		goto reply;
	}

	if (call->conn->global_catalog) {
		search_control = ldb_request_get_control(lreq, LDB_CONTROL_SEARCH_OPTIONS_OID);

		search_options = NULL;
		if (search_control) {
			search_options = talloc_get_type(search_control->data, struct ldb_search_options_control);
			search_options->search_options |= LDB_SEARCH_OPTION_PHANTOM_ROOT;
		} else {
			search_options = talloc(lreq, struct ldb_search_options_control);
			NT_STATUS_HAVE_NO_MEMORY(search_options);
			search_options->search_options = LDB_SEARCH_OPTION_PHANTOM_ROOT;
			ldb_request_add_control(lreq, LDB_CONTROL_SEARCH_OPTIONS_OID, false, search_options);
		}
	} else {
		ldb_request_add_control(lreq, DSDB_CONTROL_NO_GLOBAL_CATALOG, false, NULL);
	}

	extended_dn_control = ldb_request_get_control(lreq, LDB_CONTROL_EXTENDED_DN_OID);

	if (extended_dn_control) {
		if (extended_dn_control->data) {
			extended_dn_decoded = talloc_get_type(extended_dn_control->data, struct ldb_extended_dn_control);
			extended_type = extended_dn_decoded->type;
		} else {
			extended_type = 0;
		}
		callback_ctx->extended_type = extended_type;
	}

	notification_control = ldb_request_get_control(lreq, LDB_CONTROL_NOTIFICATION_OID);
	if (notification_control != NULL) {
		const struct ldapsrv_call *pc = NULL;
		size_t count = 0;

		for (pc = call->conn->pending_calls; pc != NULL; pc = pc->next) {
			count += 1;
		}

		if (count >= call->conn->limits.max_notifications) {
			DEBUG(10,("SearchRequest: error MaxNotificationPerConn\n"));
			result = map_ldb_error(local_ctx,
					       LDB_ERR_ADMIN_LIMIT_EXCEEDED,
					       "MaxNotificationPerConn reached",
					       &errstr);
			goto reply;
		}

		/*
		 * For now we need to do periodic retries on our own.
		 * As the dsdb_notification module will return after each run.
		 */
		call->notification.busy = true;
	}

	{
		const char *scheme = NULL;
		switch (call->conn->referral_scheme) {
		case LDAP_REFERRAL_SCHEME_LDAPS:
			scheme = "ldaps";
			break;
		default:
			scheme = "ldap";
		}
		ldb_ret = ldb_set_opaque(
			samdb,
			LDAP_REFERRAL_SCHEME_OPAQUE,
			discard_const_p(char *, scheme));
		if (ldb_ret != LDB_SUCCESS) {
			goto reply;
		}
	}

	{
		time_t timeout = call->conn->limits.search_timeout;

		if (timeout == 0
		    || (req->timelimit != 0
			&& req->timelimit < timeout))
		{
			timeout = req->timelimit;
		}
		ldb_set_timeout(samdb, lreq, timeout);
	}

	if (!call->conn->is_privileged) {
		ldb_req_mark_untrusted(lreq);
	}

	LDB_REQ_SET_LOCATION(lreq);

	ldb_ret = ldb_request(samdb, lreq);

	if (ldb_ret != LDB_SUCCESS) {
		goto reply;
	}

	ldb_ret = ldb_wait(lreq->handle, LDB_WAIT_ALL);

	if (ldb_ret == LDB_SUCCESS) {
		if (call->notification.busy) {
			/* Move/Add it to the end */
			DLIST_DEMOTE(call->conn->pending_calls, call);
			call->notification.generation =
				call->conn->service->notification.generation;

			if (callback_ctx->count != 0) {
				call->notification.generation += 1;
				ldapsrv_notification_retry_setup(call->conn->service,
								 true);
			}

			talloc_free(local_ctx);
			return NT_STATUS_OK;
		}
	}

reply:
	DLIST_REMOVE(call->conn->pending_calls, call);
	call->notification.busy = false;

	done_r = ldapsrv_init_reply(call, LDAP_TAG_SearchResultDone);
	NT_STATUS_HAVE_NO_MEMORY(done_r);

	done = &done_r->msg->r.SearchResultDone;
	done->dn = NULL;
	done->referral = NULL;

	if (result != -1) {
	} else if (ldb_ret == LDB_SUCCESS) {
		if (callback_ctx->controls) {
			done_r->msg->controls = callback_ctx->controls;
			talloc_steal(done_r->msg, callback_ctx->controls);
		}
		result = LDB_SUCCESS;
	} else {
		DEBUG(10,("SearchRequest: error\n"));
		result = map_ldb_error(local_ctx, ldb_ret, ldb_errstring(samdb),
				       &errstr);
	}

	done->resultcode = result;
	done->errormessage = (errstr?talloc_strdup(done_r, errstr):NULL);

	talloc_free(local_ctx);

	return ldapsrv_queue_reply_forced(call, done_r);
}