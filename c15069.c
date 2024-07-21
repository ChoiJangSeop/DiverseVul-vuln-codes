static int ldb_kv_index_filter(struct ldb_kv_private *ldb_kv,
			       const struct dn_list *dn_list,
			       struct ldb_kv_context *ac,
			       uint32_t *match_count,
			       enum key_truncation scope_one_truncation)
{
	struct ldb_context *ldb = ldb_module_get_ctx(ac->module);
	struct ldb_message *msg;
	struct ldb_message *filtered_msg;
	unsigned int i;
	unsigned int num_keys = 0;
	uint8_t previous_guid_key[LDB_KV_GUID_KEY_SIZE] = {0};
	struct ldb_val *keys = NULL;

	/*
	 * We have to allocate the key list (rather than just walk the
	 * caller supplied list) as the callback could change the list
	 * (by modifying an indexed attribute hosted in the in-memory
	 * index cache!)
	 */
	keys = talloc_array(ac, struct ldb_val, dn_list->count);
	if (keys == NULL) {
		return ldb_module_oom(ac->module);
	}

	if (ldb_kv->cache->GUID_index_attribute != NULL) {
		/*
		 * We speculate that the keys will be GUID based and so
		 * pre-fill in enough space for a GUID (avoiding a pile of
		 * small allocations)
		 */
		struct guid_tdb_key {
			uint8_t guid_key[LDB_KV_GUID_KEY_SIZE];
		} *key_values = NULL;

		key_values = talloc_array(keys,
					  struct guid_tdb_key,
					  dn_list->count);

		if (key_values == NULL) {
			talloc_free(keys);
			return ldb_module_oom(ac->module);
		}
		for (i = 0; i < dn_list->count; i++) {
			keys[i].data = key_values[i].guid_key;
			keys[i].length = sizeof(key_values[i].guid_key);
		}
	} else {
		for (i = 0; i < dn_list->count; i++) {
			keys[i].data = NULL;
			keys[i].length = 0;
		}
	}

	for (i = 0; i < dn_list->count; i++) {
		int ret;

		ret = ldb_kv_idx_to_key(
		    ac->module, ldb_kv, keys, &dn_list->dn[i], &keys[num_keys]);
		if (ret != LDB_SUCCESS) {
			talloc_free(keys);
			return ret;
		}

		if (ldb_kv->cache->GUID_index_attribute != NULL) {
			/*
			 * If we are in GUID index mode, then the dn_list is
			 * sorted.  If we got a duplicate, forget about it, as
			 * otherwise we would send the same entry back more
			 * than once.
			 *
			 * This is needed in the truncated DN case, or if a
			 * duplicate was forced in via
			 * LDB_FLAG_INTERNAL_DISABLE_SINGLE_VALUE_CHECK
			 */

			if (memcmp(previous_guid_key,
				   keys[num_keys].data,
				   sizeof(previous_guid_key)) == 0) {
				continue;
			}

			memcpy(previous_guid_key,
			       keys[num_keys].data,
			       sizeof(previous_guid_key));
		}
		num_keys++;
	}


	/*
	 * Now that the list is a safe copy, send the callbacks
	 */
	for (i = 0; i < num_keys; i++) {
		int ret;
		bool matched;
		msg = ldb_msg_new(ac);
		if (!msg) {
			talloc_free(keys);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		ret =
		    ldb_kv_search_key(ac->module,
				      ldb_kv,
				      keys[i],
				      msg,
				      LDB_UNPACK_DATA_FLAG_NO_VALUES_ALLOC |
				      /*
				       * The entry point ldb_kv_search_indexed is
				       * only called from the read-locked
				       * ldb_kv_search.
				       */
				      LDB_UNPACK_DATA_FLAG_READ_LOCKED);
		if (ret == LDB_ERR_NO_SUCH_OBJECT) {
			/*
			 * the record has disappeared? yes, this can
			 * happen if the entry is deleted by something
			 * operating in the callback (not another
			 * process, as we have a read lock)
			 */
			talloc_free(msg);
			continue;
		}

		if (ret != LDB_SUCCESS && ret != LDB_ERR_NO_SUCH_OBJECT) {
			/* an internal error */
			talloc_free(keys);
			talloc_free(msg);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		/*
		 * We trust the index for LDB_SCOPE_ONELEVEL
		 * unless the index key has been truncated.
		 *
		 * LDB_SCOPE_BASE is not passed in by our only caller.
		 */
		if (ac->scope == LDB_SCOPE_ONELEVEL &&
		    ldb_kv->cache->one_level_indexes &&
		    scope_one_truncation == KEY_NOT_TRUNCATED) {
			ret = ldb_match_message(ldb, msg, ac->tree,
						ac->scope, &matched);
		} else {
			ret = ldb_match_msg_error(ldb, msg,
						  ac->tree, ac->base,
						  ac->scope, &matched);
		}

		if (ret != LDB_SUCCESS) {
			talloc_free(keys);
			talloc_free(msg);
			return ret;
		}
		if (!matched) {
			talloc_free(msg);
			continue;
		}

		filtered_msg = ldb_msg_new(ac);
		if (filtered_msg == NULL) {
			TALLOC_FREE(keys);
			TALLOC_FREE(msg);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		filtered_msg->dn = talloc_steal(filtered_msg, msg->dn);

		/* filter the attributes that the user wants */
		ret = ldb_kv_filter_attrs(ldb, msg, ac->attrs, filtered_msg);

		talloc_free(msg);

		if (ret == -1) {
			TALLOC_FREE(filtered_msg);
			talloc_free(keys);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		ret = ldb_module_send_entry(ac->req, filtered_msg, NULL);
		if (ret != LDB_SUCCESS) {
			/* Regardless of success or failure, the msg
			 * is the callbacks responsiblity, and should
			 * not be talloc_free()'ed */
			ac->request_terminated = true;
			talloc_free(keys);
			return ret;
		}

		(*match_count)++;
	}

	TALLOC_FREE(keys);
	return LDB_SUCCESS;
}