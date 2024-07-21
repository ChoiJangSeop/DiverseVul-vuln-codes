int ldb_kv_search_dn1(struct ldb_module *module,
		      struct ldb_dn *dn,
		      struct ldb_message *msg,
		      unsigned int unpack_flags)
{
	void *data = ldb_module_get_private(module);
	struct ldb_kv_private *ldb_kv =
	    talloc_get_type(data, struct ldb_kv_private);
	int ret;
	uint8_t guid_key[LDB_KV_GUID_KEY_SIZE];
	struct ldb_val key = {
		.data = guid_key,
		.length = sizeof(guid_key)
	};
	TALLOC_CTX *tdb_key_ctx = NULL;

	if (ldb_kv->cache->GUID_index_attribute == NULL ||
	    ldb_dn_is_special(dn)) {

		tdb_key_ctx = talloc_new(msg);
		if (!tdb_key_ctx) {
			return ldb_module_oom(module);
		}

		/* form the key */
		key = ldb_kv_key_dn(module, tdb_key_ctx, dn);
		if (!key.data) {
			TALLOC_FREE(tdb_key_ctx);
			return LDB_ERR_OPERATIONS_ERROR;
		}
	} else {
		/*
		 * Look in the index to find the key for this DN.
		 *
		 * the tdb_key memory is allocated above, msg is just
		 * used for internal memory.
		 *
		 */
		ret = ldb_kv_key_dn_from_idx(module, ldb_kv, msg, dn, &key);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	ret = ldb_kv_search_key(module, ldb_kv, key, msg, unpack_flags);

	TALLOC_FREE(tdb_key_ctx);

	if (ret != LDB_SUCCESS) {
		return ret;
	}

	if ((unpack_flags & LDB_UNPACK_DATA_FLAG_NO_DN) == 0) {
		if (!msg->dn) {
			msg->dn = ldb_dn_copy(msg, dn);
		}
		if (!msg->dn) {
			return LDB_ERR_OPERATIONS_ERROR;
		}
	}

	return LDB_SUCCESS;
}