int ldb_kv_search(struct ldb_kv_context *ctx)
{
	struct ldb_context *ldb;
	struct ldb_module *module = ctx->module;
	struct ldb_request *req = ctx->req;
	void *data = ldb_module_get_private(module);
	struct ldb_kv_private *ldb_kv =
	    talloc_get_type(data, struct ldb_kv_private);
	int ret;

	ldb = ldb_module_get_ctx(module);

	ldb_request_set_state(req, LDB_ASYNC_PENDING);

	if (ldb_kv->kv_ops->lock_read(module) != 0) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	if (ldb_kv_cache_load(module) != 0) {
		ldb_kv->kv_ops->unlock_read(module);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	if (req->op.search.tree == NULL) {
		ldb_kv->kv_ops->unlock_read(module);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ctx->tree = req->op.search.tree;
	ctx->scope = req->op.search.scope;
	ctx->base = req->op.search.base;
	ctx->attrs = req->op.search.attrs;

	if ((req->op.search.base == NULL) || (ldb_dn_is_null(req->op.search.base) == true)) {

		/* Check what we should do with a NULL dn */
		switch (req->op.search.scope) {
		case LDB_SCOPE_BASE:
			ldb_asprintf_errstring(ldb,
					       "NULL Base DN invalid for a base search");
			ret = LDB_ERR_INVALID_DN_SYNTAX;
			break;
		case LDB_SCOPE_ONELEVEL:
			ldb_asprintf_errstring(ldb,
					       "NULL Base DN invalid for a one-level search");
			ret = LDB_ERR_INVALID_DN_SYNTAX;
			break;
		case LDB_SCOPE_SUBTREE:
		default:
			/* We accept subtree searches from a NULL base DN, ie over the whole DB */
			ret = LDB_SUCCESS;
		}
	} else if (ldb_dn_is_valid(req->op.search.base) == false) {

		/* We don't want invalid base DNs here */
		ldb_asprintf_errstring(ldb,
				       "Invalid Base DN: %s",
				       ldb_dn_get_linearized(req->op.search.base));
		ret = LDB_ERR_INVALID_DN_SYNTAX;

	} else if (req->op.search.scope == LDB_SCOPE_BASE) {

		/*
		 * If we are LDB_SCOPE_BASE, do just one search and
		 * return early.  This is critical to ensure we do not
		 * go into the index code for special DNs, as that
		 * will try to look up an index record for a special
		 * record (which doesn't exist).
		 */
		ret = ldb_kv_search_and_return_base(ldb_kv, ctx);

		ldb_kv->kv_ops->unlock_read(module);

		return ret;

	} else if (ldb_kv->check_base) {
		/*
		 * This database has been marked as
		 * 'checkBaseOnSearch', so do a spot check of the base
		 * dn.  Also optimise the subsequent filter by filling
		 * in the ctx->base to be exactly case correct
		 */
		ret = ldb_kv_search_base(
		    module, ctx, req->op.search.base, &ctx->base);

		if (ret == LDB_ERR_NO_SUCH_OBJECT) {
			ldb_asprintf_errstring(ldb,
					       "No such Base DN: %s",
					       ldb_dn_get_linearized(req->op.search.base));
		}

	} else {
		/* If we are not checking the base DN life is easy */
		ret = LDB_SUCCESS;
	}

	if (ret == LDB_SUCCESS) {
		uint32_t match_count = 0;

		ret = ldb_kv_search_indexed(ctx, &match_count);
		if (ret == LDB_ERR_NO_SUCH_OBJECT) {
			/* Not in the index, therefore OK! */
			ret = LDB_SUCCESS;

		}
		/* Check if we got just a normal error.
		 * In that case proceed to a full search unless we got a
		 * callback error */
		if (!ctx->request_terminated && ret != LDB_SUCCESS) {
			/* Not indexed, so we need to do a full scan */
			if (ldb_kv->warn_unindexed ||
			    ldb_kv->disable_full_db_scan) {
				/* useful for debugging when slow performance
				 * is caused by unindexed searches */
				char *expression = ldb_filter_from_tree(ctx, ctx->tree);
				ldb_debug(ldb, LDB_DEBUG_ERROR, "ldb FULL SEARCH: %s SCOPE: %s DN: %s",
							expression,
							req->op.search.scope==LDB_SCOPE_BASE?"base":
							req->op.search.scope==LDB_SCOPE_ONELEVEL?"one":
							req->op.search.scope==LDB_SCOPE_SUBTREE?"sub":"UNKNOWN",
							ldb_dn_get_linearized(req->op.search.base));

				talloc_free(expression);
			}

			if (match_count != 0) {
				/* the indexing code gave an error
				 * after having returned at least one
				 * entry. This means the indexes are
				 * corrupt or a database record is
				 * corrupt. We cannot continue with a
				 * full search or we may return
				 * duplicate entries
				 */
				ldb_kv->kv_ops->unlock_read(module);
				return LDB_ERR_OPERATIONS_ERROR;
			}

			if (ldb_kv->disable_full_db_scan) {
				ldb_set_errstring(ldb,
						  "ldb FULL SEARCH disabled");
				ldb_kv->kv_ops->unlock_read(module);
				return LDB_ERR_INAPPROPRIATE_MATCHING;
			}

			ret = ldb_kv_search_full(ctx);
			if (ret != LDB_SUCCESS) {
				ldb_set_errstring(ldb, "Indexed and full searches both failed!\n");
			}
		}
	}

	ldb_kv->kv_ops->unlock_read(module);

	return ret;
}