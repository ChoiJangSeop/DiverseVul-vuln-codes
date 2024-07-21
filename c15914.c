static int anr_search(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct ldb_parse_tree *anr_tree;
	struct ldb_request *down_req;
	struct anr_context *ac;
	int ret;

	ldb = ldb_module_get_ctx(module);

	ac = talloc(req, struct anr_context);
	if (!ac) {
		return ldb_oom(ldb);
	}

	ac->module = module;
	ac->req = req;
	ac->found_anr = false;

#if 0
	printf("oldanr : %s\n", ldb_filter_from_tree (0, req->op.search.tree));
#endif

	ret = anr_replace_subtrees(ac, req->op.search.tree, "anr", &anr_tree);
	if (ret != LDB_SUCCESS) {
		return ldb_operr(ldb);
	}

	if (!ac->found_anr) {
		talloc_free(ac);
		return ldb_next_request(module, req);
	}

	ret = ldb_build_search_req_ex(&down_req,
					ldb, ac,
					req->op.search.base,
					req->op.search.scope,
					anr_tree,
					req->op.search.attrs,
					req->controls,
					ac, anr_search_callback,
					req);
	LDB_REQ_SET_LOCATION(down_req);
	if (ret != LDB_SUCCESS) {
		return ldb_operr(ldb);
	}
	talloc_steal(down_req, anr_tree);

	return ldb_next_request(module, down_req);
}