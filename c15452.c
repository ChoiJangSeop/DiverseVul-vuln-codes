struct ldb_request *map_build_fixup_req(struct map_context *ac,
					struct ldb_dn *olddn,
					struct ldb_dn *newdn,
					void *context,
					ldb_map_callback_t callback)
{
	struct ldb_context *ldb;
	struct ldb_request *req;
	struct ldb_message *msg;
	const char *dn;
	int ret;

	ldb = ldb_module_get_ctx(ac->module);

	/* Prepare message */
	msg = ldb_msg_new(ac);
	if (msg == NULL) {
		map_oom(ac->module);
		return NULL;
	}

	/* Update local 'IS_MAPPED' to the new remote DN */
	msg->dn = ldb_dn_copy(msg, olddn);
	dn = ldb_dn_alloc_linearized(msg, newdn);
	if ( ! dn || ! ldb_dn_validate(msg->dn)) {
		goto failed;
	}
	if (ldb_msg_add_empty(msg, IS_MAPPED, LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		goto failed;
	}
	if (ldb_msg_add_string(msg, IS_MAPPED, dn) != 0) {
		goto failed;
	}

	/* Prepare request */
	ret = ldb_build_mod_req(&req, ldb,
				ac, msg, NULL,
				context, callback,
				ac->req);
	LDB_REQ_SET_LOCATION(req);
	if (ret != LDB_SUCCESS) {
		goto failed;
	}
	talloc_steal(req, msg);

	return req;
failed:
	talloc_free(msg);
	return NULL;
}