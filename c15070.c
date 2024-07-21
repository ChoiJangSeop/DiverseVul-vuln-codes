static int ldb_kv_handle_request(struct ldb_module *module,
				 struct ldb_request *req)
{
	struct ldb_control *control_permissive;
	struct ldb_context *ldb;
	struct tevent_context *ev;
	struct ldb_kv_context *ac;
	struct tevent_timer *te;
	struct timeval tv;
	unsigned int i;

	ldb = ldb_module_get_ctx(module);

	control_permissive = ldb_request_get_control(req,
					LDB_CONTROL_PERMISSIVE_MODIFY_OID);

	for (i = 0; req->controls && req->controls[i]; i++) {
		if (req->controls[i]->critical &&
		    req->controls[i] != control_permissive) {
			ldb_asprintf_errstring(ldb, "Unsupported critical extension %s",
					       req->controls[i]->oid);
			return LDB_ERR_UNSUPPORTED_CRITICAL_EXTENSION;
		}
	}

	if (req->starttime == 0 || req->timeout == 0) {
		ldb_set_errstring(ldb, "Invalid timeout settings");
		return LDB_ERR_TIME_LIMIT_EXCEEDED;
	}

	ev = ldb_handle_get_event_context(req->handle);

	ac = talloc_zero(ldb, struct ldb_kv_context);
	if (ac == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ac->module = module;
	ac->req = req;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	te = tevent_add_timer(ev, ac, tv, ldb_kv_callback, ac);
	if (NULL == te) {
		talloc_free(ac);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	if (req->timeout > 0) {
		tv.tv_sec = req->starttime + req->timeout;
		tv.tv_usec = 0;
		ac->timeout_event =
		    tevent_add_timer(ev, ac, tv, ldb_kv_timeout, ac);
		if (NULL == ac->timeout_event) {
			talloc_free(ac);
			return LDB_ERR_OPERATIONS_ERROR;
		}
	}

	/* set a spy so that we do not try to use the request context
	 * if it is freed before ltdb_callback fires */
	ac->spy = talloc(req, struct ldb_kv_req_spy);
	if (NULL == ac->spy) {
		talloc_free(ac);
		return LDB_ERR_OPERATIONS_ERROR;
	}
	ac->spy->ctx = ac;

	talloc_set_destructor((TALLOC_CTX *)ac->spy, ldb_kv_request_destructor);

	return LDB_SUCCESS;
}