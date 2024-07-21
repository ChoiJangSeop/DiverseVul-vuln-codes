static int ldb_kv_add_internal(struct ldb_module *module,
			       struct ldb_kv_private *ldb_kv,
			       const struct ldb_message *msg,
			       bool check_single_value)
{
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	int ret = LDB_SUCCESS;
	unsigned int i;

	for (i=0;i<msg->num_elements;i++) {
		struct ldb_message_element *el = &msg->elements[i];
		const struct ldb_schema_attribute *a = ldb_schema_attribute_by_name(ldb, el->name);

		if (el->num_values == 0) {
			ldb_asprintf_errstring(ldb, "attribute '%s' on '%s' specified, but with 0 values (illegal)",
					       el->name, ldb_dn_get_linearized(msg->dn));
			return LDB_ERR_CONSTRAINT_VIOLATION;
		}
		if (check_single_value && el->num_values > 1 &&
		    ldb_kv_single_valued(a, el)) {
			ldb_asprintf_errstring(ldb, "SINGLE-VALUE attribute %s on %s specified more than once",
					       el->name, ldb_dn_get_linearized(msg->dn));
			return LDB_ERR_CONSTRAINT_VIOLATION;
		}

		/* Do not check "@ATTRIBUTES" for duplicated values */
		if (ldb_dn_is_special(msg->dn) &&
		    ldb_dn_check_special(msg->dn, LDB_KV_ATTRIBUTES)) {
			continue;
		}

		if (check_single_value &&
		    !(el->flags &
		      LDB_FLAG_INTERNAL_DISABLE_SINGLE_VALUE_CHECK)) {
			struct ldb_val *duplicate = NULL;

			ret = ldb_msg_find_duplicate_val(ldb, discard_const(msg),
							 el, &duplicate, 0);
			if (ret != LDB_SUCCESS) {
				return ret;
			}
			if (duplicate != NULL) {
				ldb_asprintf_errstring(
					ldb,
					"attribute '%s': value '%.*s' on '%s' "
					"provided more than once in ADD object",
					el->name,
					(int)duplicate->length,
					duplicate->data,
					ldb_dn_get_linearized(msg->dn));
				return LDB_ERR_ATTRIBUTE_OR_VALUE_EXISTS;
			}
		}
	}

	ret = ldb_kv_store(module, msg, TDB_INSERT);
	if (ret != LDB_SUCCESS) {
		/*
		 * Try really hard to get the right error code for
		 * a re-add situation, as this can matter!
		 */
		if (ret == LDB_ERR_CONSTRAINT_VIOLATION) {
			int ret2;
			struct ldb_dn *dn2 = NULL;
			TALLOC_CTX *mem_ctx = talloc_new(module);
			if (mem_ctx == NULL) {
				return ldb_module_operr(module);
			}
			ret2 =
			    ldb_kv_search_base(module, mem_ctx, msg->dn, &dn2);
			TALLOC_FREE(mem_ctx);
			if (ret2 == LDB_SUCCESS) {
				ret = LDB_ERR_ENTRY_ALREADY_EXISTS;
			}
		}
		if (ret == LDB_ERR_ENTRY_ALREADY_EXISTS) {
			ldb_asprintf_errstring(ldb,
					       "Entry %s already exists",
					       ldb_dn_get_linearized(msg->dn));
		}
		return ret;
	}

	ret = ldb_kv_index_add_new(module, ldb_kv, msg);
	if (ret != LDB_SUCCESS) {
		/*
		 * If we failed to index, delete the message again.
		 *
		 * This is particularly important for the GUID index
		 * case, which will only fail for a duplicate DN
		 * in the index add.
		 *
		 * Note that the caller may not cancel the transation
		 * and this means the above add might really show up!
		 */
		ldb_kv_delete_noindex(module, msg);
		return ret;
	}

	ret = ldb_kv_modified(module, msg->dn);

	return ret;
}