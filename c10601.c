int cil_build_ast(struct cil_db *db, struct cil_tree_node *parse_tree, struct cil_tree_node *ast)
{
	int rc = SEPOL_ERR;
	struct cil_args_build extra_args;

	if (db == NULL || parse_tree == NULL || ast == NULL) {
		goto exit;
	}

	extra_args.ast = ast;
	extra_args.db = db;
	extra_args.tunif = NULL;
	extra_args.in = NULL;
	extra_args.macro = NULL;
	extra_args.boolif = NULL;

	rc = cil_tree_walk(parse_tree, __cil_build_ast_node_helper, __cil_build_ast_first_child_helper, __cil_build_ast_last_child_helper, &extra_args);
	if (rc != SEPOL_OK) {
		goto exit;
	}

	return SEPOL_OK;

exit:
	return rc;
}