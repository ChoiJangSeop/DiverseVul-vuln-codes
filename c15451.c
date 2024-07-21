uint64_t winsdb_set_maxVersion(struct winsdb_handle *h, uint64_t newMaxVersion)
{
	int trans;
	int ret;
	struct ldb_dn *dn;
	struct ldb_result *res = NULL;
	struct ldb_message *msg = NULL;
	struct ldb_context *wins_db = h->ldb;
	TALLOC_CTX *tmp_ctx = talloc_new(wins_db);
	uint64_t oldMaxVersion = 0;

	trans = ldb_transaction_start(wins_db);
	if (trans != LDB_SUCCESS) goto failed;

	dn = ldb_dn_new(tmp_ctx, wins_db, "CN=VERSION");
	if (!dn) goto failed;

	/* find the record in the WINS database */
	ret = ldb_search(wins_db, tmp_ctx, &res, dn, LDB_SCOPE_BASE, NULL, NULL);
	if (ret != LDB_SUCCESS) goto failed;
	if (res->count > 1) goto failed;

	if (res->count == 1) {
		oldMaxVersion = ldb_msg_find_attr_as_uint64(res->msgs[0], "maxVersion", 0);
	}

	if (newMaxVersion == 0) {
		newMaxVersion = oldMaxVersion + 1;
	} else {
		newMaxVersion = MAX(oldMaxVersion, newMaxVersion);
	}

	msg = ldb_msg_new(tmp_ctx);
	if (!msg) goto failed;
	msg->dn = dn;


	ret = ldb_msg_add_empty(msg, "objectClass", LDB_FLAG_MOD_REPLACE, NULL);
	if (ret != LDB_SUCCESS) goto failed;
	ret = ldb_msg_add_string(msg, "objectClass", "winsMaxVersion");
	if (ret != LDB_SUCCESS) goto failed;
	ret = ldb_msg_add_empty(msg, "maxVersion", LDB_FLAG_MOD_REPLACE, NULL);
	if (ret != LDB_SUCCESS) goto failed;
	ret = ldb_msg_add_fmt(msg, "maxVersion", "%llu", (long long)newMaxVersion);
	if (ret != LDB_SUCCESS) goto failed;

	ret = ldb_modify(wins_db, msg);
	if (ret != LDB_SUCCESS) ret = ldb_add(wins_db, msg);
	if (ret != LDB_SUCCESS) goto failed;

	trans = ldb_transaction_commit(wins_db);
	if (trans != LDB_SUCCESS) goto failed;

	talloc_free(tmp_ctx);
	return newMaxVersion;

failed:
	if (trans == LDB_SUCCESS) ldb_transaction_cancel(wins_db);
	talloc_free(tmp_ctx);
	return 0;
}