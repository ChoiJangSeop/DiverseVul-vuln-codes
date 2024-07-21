static int ldb_kv_rename(struct ldb_kv_context *ctx)
{
	struct ldb_module *module = ctx->module;
	void *data = ldb_module_get_private(module);
	struct ldb_kv_private *ldb_kv =
	    talloc_get_type(data, struct ldb_kv_private);
	struct ldb_request *req = ctx->req;
	struct ldb_message *msg;
	int ret = LDB_SUCCESS;
	struct ldb_val  key, key_old;
	struct ldb_dn *db_dn;

	ldb_request_set_state(req, LDB_ASYNC_PENDING);

	if (ldb_kv_cache_load(ctx->module) != 0) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	msg = ldb_msg_new(ctx);
	if (msg == NULL) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	/* we need to fetch the old record to re-add under the new name */
	ret = ldb_kv_search_dn1(module,
				req->op.rename.olddn,
				msg,
				LDB_UNPACK_DATA_FLAG_NO_DATA_ALLOC);
	if (ret != LDB_SUCCESS) {
		/* not finding the old record is an error */
		return ret;
	}

	/* We need to, before changing the DB, check if the new DN
	 * exists, so we can return this error to the caller with an
	 * unmodified DB
	 *
	 * Even in GUID index mode we use ltdb_key_dn() as we are
	 * trying to figure out if this is just a case rename
	 */
	key = ldb_kv_key_dn(module, msg, req->op.rename.newdn);
	if (!key.data) {
		talloc_free(msg);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	key_old = ldb_kv_key_dn(module, msg, req->op.rename.olddn);
	if (!key_old.data) {
		talloc_free(msg);
		talloc_free(key.data);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	/*
	 * Only declare a conflict if the new DN already exists,
	 * and it isn't a case change on the old DN
	 */
	if (key_old.length != key.length
	    || memcmp(key.data, key_old.data, key.length) != 0) {
		ret = ldb_kv_search_base(
		    module, msg, req->op.rename.newdn, &db_dn);
		if (ret == LDB_SUCCESS) {
			ret = LDB_ERR_ENTRY_ALREADY_EXISTS;
		} else if (ret == LDB_ERR_NO_SUCH_OBJECT) {
			ret = LDB_SUCCESS;
		}
	}

	/* finding the new record already in the DB is an error */

	if (ret == LDB_ERR_ENTRY_ALREADY_EXISTS) {
		ldb_asprintf_errstring(ldb_module_get_ctx(module),
				       "Entry %s already exists",
				       ldb_dn_get_linearized(req->op.rename.newdn));
	}
	if (ret != LDB_SUCCESS) {
		talloc_free(key_old.data);
		talloc_free(key.data);
		talloc_free(msg);
		return ret;
	}

	talloc_free(key_old.data);
	talloc_free(key.data);

	/* Always delete first then add, to avoid conflicts with
	 * unique indexes. We rely on the transaction to make this
	 * atomic
	 */
	ret = ldb_kv_delete_internal(module, msg->dn);
	if (ret != LDB_SUCCESS) {
		talloc_free(msg);
		return ret;
	}

	msg->dn = ldb_dn_copy(msg, req->op.rename.newdn);
	if (msg->dn == NULL) {
		talloc_free(msg);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	/* We don't check single value as we can have more than 1 with
	 * deleted attributes. We could go through all elements but that's
	 * maybe not the most efficient way
	 */
	ret = ldb_kv_add_internal(module, ldb_kv, msg, false);

	talloc_free(msg);

	return ret;
}