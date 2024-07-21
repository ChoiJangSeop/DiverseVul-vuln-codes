static int search_func(_UNUSED_ struct ldb_kv_private *ldb_kv,
		       struct ldb_val key,
		       struct ldb_val val,
		       void *state)
{
	struct ldb_context *ldb;
	struct ldb_kv_context *ac;
	struct ldb_message *msg, *filtered_msg;
	int ret;
	bool matched;

	ac = talloc_get_type(state, struct ldb_kv_context);
	ldb = ldb_module_get_ctx(ac->module);

	/*
	 * We want to skip @ records early in a search full scan
	 *
	 * @ records like @IDXLIST are only available via a base
	 * search on the specific name but the method by which they
	 * were excluded was expensive, after the unpack the DN is
	 * exploded and ldb_match_msg_error() would reject it for
	 * failing to match the scope.
	 *
	 * ldb_kv_key_is_normal_record() uses the fact that @ records
	 * have the DN=@ prefix on their TDB/LMDB key to quickly
	 * exclude them from consideration.
	 *
	 * (any other non-records are also excluded by the same key
	 * match)
	 */

	if (ldb_kv_key_is_normal_record(key) == false) {
		return 0;
	}

	msg = ldb_msg_new(ac);
	if (!msg) {
		ac->error = LDB_ERR_OPERATIONS_ERROR;
		return -1;
	}

	/* unpack the record */
	ret = ldb_unpack_data_flags(ldb, &val, msg,
				    LDB_UNPACK_DATA_FLAG_NO_VALUES_ALLOC);
	if (ret == -1) {
		talloc_free(msg);
		ac->error = LDB_ERR_OPERATIONS_ERROR;
		return -1;
	}

	if (!msg->dn) {
		msg->dn = ldb_dn_new(msg, ldb,
				     (char *)key.data + 3);
		if (msg->dn == NULL) {
			talloc_free(msg);
			ac->error = LDB_ERR_OPERATIONS_ERROR;
			return -1;
		}
	}

	/* see if it matches the given expression */
	ret = ldb_match_msg_error(ldb, msg,
				  ac->tree, ac->base, ac->scope, &matched);
	if (ret != LDB_SUCCESS) {
		talloc_free(msg);
		ac->error = LDB_ERR_OPERATIONS_ERROR;
		return -1;
	}
	if (!matched) {
		talloc_free(msg);
		return 0;
	}

	filtered_msg = ldb_msg_new(ac);
	if (filtered_msg == NULL) {
		TALLOC_FREE(msg);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	filtered_msg->dn = talloc_steal(filtered_msg, msg->dn);

	/* filter the attributes that the user wants */
	ret = ldb_kv_filter_attrs(ldb, msg, ac->attrs, filtered_msg);
	talloc_free(msg);

	if (ret == -1) {
		TALLOC_FREE(filtered_msg);
		ac->error = LDB_ERR_OPERATIONS_ERROR;
		return -1;
	}

	ret = ldb_module_send_entry(ac->req, filtered_msg, NULL);
	if (ret != LDB_SUCCESS) {
		ac->request_terminated = true;
		/* the callback failed, abort the operation */
		ac->error = LDB_ERR_OPERATIONS_ERROR;
		return -1;
	}

	return 0;
}