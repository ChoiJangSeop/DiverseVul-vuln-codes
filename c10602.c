int __cil_build_ast_last_child_helper(struct cil_tree_node *parse_current, void *extra_args)
{
	struct cil_args_build *args = extra_args;
	struct cil_tree_node *ast = args->ast;

	if (ast->flavor == CIL_ROOT) {
		return SEPOL_OK;
	}

	args->ast = ast->parent;

	if (ast->flavor == CIL_TUNABLEIF) {
		args->tunif = NULL;
	}

	if (ast->flavor == CIL_IN) {
		args->in = NULL;
	}

	if (ast->flavor == CIL_MACRO) {
		args->macro = NULL;
	}

	if (ast->flavor == CIL_BOOLEANIF) {
		args->boolif = NULL;
	}

	// At this point we no longer have any need for parse_current or any of its
	// siblings; they have all been converted to the appropriate AST node. The
	// full parse tree will get deleted elsewhere, but in an attempt to
	// minimize memory usage (of which the parse tree uses a lot), start
	// deleting the parts we don't need now.
	cil_tree_children_destroy(parse_current->parent);

	return SEPOL_OK;
}