static int anr_replace_value(struct anr_context *ac,
			     TALLOC_CTX *mem_ctx,
			     struct ldb_val *match,
			     struct ldb_parse_tree **ntree)
{
	struct ldb_parse_tree *tree = NULL;
	struct ldb_module *module = ac->module;
	struct ldb_parse_tree *match_tree;
	struct dsdb_attribute *cur;
	const struct dsdb_schema *schema;
	struct ldb_context *ldb;
	uint8_t *p;
	enum ldb_parse_op op;

	ldb = ldb_module_get_ctx(module);

	schema = dsdb_get_schema(ldb, ac);
	if (!schema) {
		ldb_asprintf_errstring(ldb, "no schema with which to construct anr filter");
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ac->found_anr = true;

	if (match->length > 1 && match->data[0] == '=') {
		struct ldb_val *match2 = talloc(mem_ctx, struct ldb_val);
		if (match2 == NULL){
			return ldb_oom(ldb);
		}
		*match2 = data_blob_const(match->data+1, match->length - 1);
		match = match2;
		op = LDB_OP_EQUALITY;
	} else {
		op = LDB_OP_SUBSTRING;
	}
	for (cur = schema->attributes; cur; cur = cur->next) {
		if (!(cur->searchFlags & SEARCH_FLAG_ANR)) continue;
		match_tree = make_match_tree(module, mem_ctx, op, cur->lDAPDisplayName, match);

		if (tree) {
			/* Inject an 'or' with the current tree */
			tree = make_parse_list(module, mem_ctx,  LDB_OP_OR, tree, match_tree);
			if (tree == NULL) {
				return ldb_oom(ldb);
			}
		} else {
			tree = match_tree;
		}
	}

	
	/* If the search term has a space in it, 
	   split it up at the first space.  */
	
	p = memchr(match->data, ' ', match->length);

	if (p) {
		struct ldb_parse_tree *first_split_filter, *second_split_filter, *split_filters, *match_tree_1, *match_tree_2;
		struct ldb_val *first_match = talloc(tree, struct ldb_val);
		struct ldb_val *second_match = talloc(tree, struct ldb_val);
		if (!first_match || !second_match) {
			return ldb_oom(ldb);
		}
		*first_match = data_blob_const(match->data, p-match->data);
		*second_match = data_blob_const(p+1, match->length - (p-match->data) - 1);
		
		/* Add (|(&(givenname=first)(sn=second))(&(givenname=second)(sn=first))) */

		match_tree_1 = make_match_tree(module, mem_ctx, op, "givenName", first_match);
		match_tree_2 = make_match_tree(module, mem_ctx, op, "sn", second_match);

		first_split_filter = make_parse_list(module, ac,  LDB_OP_AND, match_tree_1, match_tree_2);
		if (first_split_filter == NULL){
			return ldb_oom(ldb);
		}
		
		match_tree_1 = make_match_tree(module, mem_ctx, op, "sn", first_match);
		match_tree_2 = make_match_tree(module, mem_ctx, op, "givenName", second_match);

		second_split_filter = make_parse_list(module, ac,  LDB_OP_AND, match_tree_1, match_tree_2);
		if (second_split_filter == NULL){
			return ldb_oom(ldb);
		}

		split_filters = make_parse_list(module, mem_ctx,  LDB_OP_OR, 
						first_split_filter, second_split_filter);
		if (split_filters == NULL) {
			return ldb_oom(ldb);
		}

		if (tree) {
			/* Inject an 'or' with the current tree */
			tree = make_parse_list(module, mem_ctx,  LDB_OP_OR, tree, split_filters);
		} else {
			tree = split_filters;
		}
	}
	*ntree = tree;
	return LDB_SUCCESS;
}