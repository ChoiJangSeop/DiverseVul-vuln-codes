static int rdn_rename_callback(struct ldb_request *req, struct ldb_reply *ares)
{
	struct ldb_context *ldb;
	struct rename_context *ac;
	struct ldb_request *mod_req;
	const char *rdn_name;
	const struct ldb_schema_attribute *a = NULL;
	const struct ldb_val *rdn_val_p;
	struct ldb_val rdn_val;
	struct ldb_message *msg;
	int ret;

	ac = talloc_get_type(req->context, struct rename_context);
	ldb = ldb_module_get_ctx(ac->module);

	if (!ares) {
		goto error;
	}

	if (ares->type == LDB_REPLY_REFERRAL) {
		return ldb_module_send_referral(ac->req, ares->referral);
	}

	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	/* the only supported reply right now is a LDB_REPLY_DONE */
	if (ares->type != LDB_REPLY_DONE) {
		goto error;
	}

	/* save reply for caller */
	ac->ares = talloc_steal(ac, ares);

	msg = ldb_msg_new(ac);
	if (msg == NULL) {
		goto error;
	}
	msg->dn = ldb_dn_copy(msg, ac->req->op.rename.newdn);
	if (msg->dn == NULL) {
		goto error;
	}

	rdn_name = ldb_dn_get_rdn_name(ac->req->op.rename.newdn);
	if (rdn_name == NULL) {
		goto error;
	}

	a = ldb_schema_attribute_by_name(ldb, rdn_name);
	if (a == NULL) {
		goto error;
	}

	if (a->name != NULL && strcmp(a->name, "*") != 0) {
		rdn_name = a->name;
	}

	rdn_val_p = ldb_dn_get_rdn_val(msg->dn);
	if (rdn_val_p == NULL) {
		goto error;
	}
	if (rdn_val_p->length == 0) {
		ldb_asprintf_errstring(ldb, "Empty RDN value on %s not permitted!",
				       ldb_dn_get_linearized(req->op.rename.olddn));
		return ldb_module_done(ac->req, NULL, NULL,
				       LDB_ERR_NAMING_VIOLATION);
	}
	rdn_val = ldb_val_dup(msg, rdn_val_p);

	if (ldb_msg_add_empty(msg, rdn_name, LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		goto error;
	}
	if (ldb_msg_add_value(msg, rdn_name, &rdn_val, NULL) != 0) {
		goto error;
	}
	if (ldb_msg_add_empty(msg, "name", LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		goto error;
	}
	if (ldb_msg_add_value(msg, "name", &rdn_val, NULL) != 0) {
		goto error;
	}

	ret = ldb_build_mod_req(&mod_req, ldb,
				ac, msg, NULL,
				ac, rdn_modify_callback,
				req);
	if (ret != LDB_SUCCESS) {
		return ldb_module_done(ac->req, NULL, NULL, ret);
	}
	talloc_steal(mod_req, msg);

	/* go on with the call chain */
	return ldb_next_request(ac->module, mod_req);

error:
	return ldb_module_done(ac->req, NULL, NULL, LDB_ERR_OPERATIONS_ERROR);
}