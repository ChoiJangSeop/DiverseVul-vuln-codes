int __cil_build_ast_first_child_helper(__attribute__((unused)) struct cil_tree_node *parse_current, void *extra_args)
{
	struct cil_args_build *args = extra_args;
	struct cil_tree_node *ast = args->ast;

	if (ast->flavor == CIL_TUNABLEIF) {
		args->tunif = ast;
	}

	if (ast->flavor == CIL_IN) {
		args->in = ast;
	}

	if (ast->flavor == CIL_MACRO) {
		args->macro = ast;
	}

	if (ast->flavor == CIL_BOOLEANIF) {
		args->boolif = ast;
	}

	return SEPOL_OK;
}