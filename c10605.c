int __cil_resolve_ast_node_helper(struct cil_tree_node *node, uint32_t *finished, void *extra_args)
{
	int rc = SEPOL_OK;
	struct cil_args_resolve *args = extra_args;
	enum cil_pass pass = args->pass;
	struct cil_tree_node *block = args->block;
	struct cil_tree_node *macro = args->macro;
	struct cil_tree_node *optional = args->optional;
	struct cil_tree_node *boolif = args->boolif;

	if (node == NULL) {
		goto exit;
	}

	if (block != NULL) {
		if (node->flavor == CIL_CAT ||
		    node->flavor == CIL_SENS) {
			cil_tree_log(node, CIL_ERR, "%s statement is not allowed in blocks", cil_node_to_string(node));
			rc = SEPOL_ERR;
			goto exit;
		}
	}

	if (macro != NULL) {
		if (node->flavor == CIL_BLOCK ||
		    node->flavor == CIL_BLOCKINHERIT ||
		    node->flavor == CIL_BLOCKABSTRACT ||
		    node->flavor == CIL_MACRO) {
			cil_tree_log(node, CIL_ERR, "%s statement is not allowed in macros", cil_node_to_string(node));
			rc = SEPOL_ERR;
			goto exit;
		}
	}

	if (optional != NULL) {
		if (node->flavor == CIL_TUNABLE ||
		    node->flavor == CIL_MACRO) {
			/* tuanbles and macros are not allowed in optionals*/
			cil_tree_log(node, CIL_ERR, "%s statement is not allowed in optionals", cil_node_to_string(node));
			rc = SEPOL_ERR;
			goto exit;
		}
	}

	if (boolif != NULL) {
		if (node->flavor != CIL_TUNABLEIF &&
			node->flavor != CIL_CALL &&
			node->flavor != CIL_CONDBLOCK &&
			node->flavor != CIL_AVRULE &&
			node->flavor != CIL_TYPE_RULE &&
			node->flavor != CIL_NAMETYPETRANSITION) {
			rc = SEPOL_ERR;
		} else if (node->flavor == CIL_AVRULE) {
			struct cil_avrule *rule = node->data;
			if (rule->rule_kind == CIL_AVRULE_NEVERALLOW) {
				rc = SEPOL_ERR;
			}
		}
		if (rc == SEPOL_ERR) {
			if (((struct cil_booleanif*)boolif->data)->preserved_tunable) {
				cil_tree_log(node, CIL_ERR, "%s statement is not allowed in booleanifs (tunableif treated as a booleanif)", cil_node_to_string(node));
			} else {
				cil_tree_log(node, CIL_ERR, "%s statement is not allowed in booleanifs", cil_node_to_string(node));
			}
			goto exit;
		}
	}

	if (node->flavor == CIL_MACRO) {
		if (pass != CIL_PASS_TIF && pass != CIL_PASS_MACRO) {
			*finished = CIL_TREE_SKIP_HEAD;
			rc = SEPOL_OK;
			goto exit;
		}
	}

	if (node->flavor == CIL_BLOCK && ((((struct cil_block*)node->data)->is_abstract == CIL_TRUE) && (pass > CIL_PASS_BLKABS))) {
		*finished = CIL_TREE_SKIP_HEAD;
		rc = SEPOL_OK;
		goto exit;
	}

	rc = __cil_resolve_ast_node(node, extra_args);
	if (rc == SEPOL_ENOENT) {
		enum cil_log_level lvl = CIL_ERR;

		if (optional != NULL) {
			lvl = CIL_INFO;

			struct cil_optional *opt = (struct cil_optional *)optional->data;
			struct cil_tree_node *opt_node = NODE(opt);;
			/* disable an optional if something failed to resolve */
			opt->enabled = CIL_FALSE;
			cil_tree_log(node, lvl, "Failed to resolve %s statement", cil_node_to_string(node));
			cil_tree_log(opt_node, lvl, "Disabling optional '%s'", opt->datum.name);
			rc = SEPOL_OK;
			goto exit;
		}

		cil_tree_log(node, lvl, "Failed to resolve %s statement", cil_node_to_string(node));
		goto exit;
	}

	return rc;

exit:
	return rc;
}