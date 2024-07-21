int __cil_build_ast_node_helper(struct cil_tree_node *parse_current, uint32_t *finished, void *extra_args)
{
	struct cil_args_build *args = extra_args;
	struct cil_db *db = args->db;
	struct cil_tree_node *ast_current = args->ast;
	struct cil_tree_node *tunif = args->tunif;
	struct cil_tree_node *in = args->in;
	struct cil_tree_node *macro = args->macro;
	struct cil_tree_node *boolif = args->boolif;
	struct cil_tree_node *ast_node = NULL;
	int rc = SEPOL_ERR;

	if (parse_current->parent->cl_head != parse_current) {
		/* ignore anything that isn't following a parenthesis */
		rc = SEPOL_OK;
		goto exit;
	} else if (parse_current->data == NULL) {
		/* the only time parenthesis can immediately following parenthesis is if
		 * the parent is the root node */
		if (parse_current->parent->parent == NULL) {
			rc = SEPOL_OK;
		} else {
			cil_tree_log(parse_current, CIL_ERR, "Keyword expected after open parenthesis");
		}
		goto exit;
	}

	if (tunif != NULL) {
		if (parse_current->data == CIL_KEY_TUNABLE) {
			rc = SEPOL_ERR;
			cil_tree_log(parse_current, CIL_ERR, "Found tunable");
			cil_log(CIL_ERR, "Tunables cannot be defined within tunableif statement\n");
			goto exit;
		}
	}

	if (in != NULL) {
		if (parse_current->data == CIL_KEY_IN) {
			rc = SEPOL_ERR;
			cil_tree_log(parse_current, CIL_ERR, "Found in-statement");
			cil_log(CIL_ERR, "in-statements cannot be defined within in-statements\n");
			goto exit;
		}
	}

	if (macro != NULL) {
		if (parse_current->data == CIL_KEY_TUNABLE ||
			parse_current->data == CIL_KEY_IN ||
			parse_current->data == CIL_KEY_BLOCK ||
			parse_current->data == CIL_KEY_BLOCKINHERIT ||
			parse_current->data == CIL_KEY_BLOCKABSTRACT ||
			parse_current->data == CIL_KEY_MACRO) {
			rc = SEPOL_ERR;
			cil_tree_log(parse_current, CIL_ERR, "%s is not allowed in macros", (char *)parse_current->data);
			goto exit;
		}
	}

	if (boolif != NULL) {
		if (parse_current->data != CIL_KEY_TUNABLEIF &&
			parse_current->data != CIL_KEY_CALL &&
			parse_current->data != CIL_KEY_CONDTRUE &&
			parse_current->data != CIL_KEY_CONDFALSE &&
			parse_current->data != CIL_KEY_ALLOW &&
			parse_current->data != CIL_KEY_DONTAUDIT &&
			parse_current->data != CIL_KEY_AUDITALLOW &&
			parse_current->data != CIL_KEY_TYPETRANSITION &&
			parse_current->data != CIL_KEY_TYPECHANGE &&
			parse_current->data != CIL_KEY_TYPEMEMBER) {
			rc = SEPOL_ERR;
			cil_tree_log(parse_current, CIL_ERR, "Found %s", (char*)parse_current->data);
			if (((struct cil_booleanif*)boolif->data)->preserved_tunable) {
				cil_log(CIL_ERR, "%s cannot be defined within tunableif statement (treated as a booleanif due to preserve-tunables)\n",
						(char*)parse_current->data);
			} else {
				cil_log(CIL_ERR, "%s cannot be defined within booleanif statement\n",
						(char*)parse_current->data);
			}
			goto exit;
		}
	}

	cil_tree_node_init(&ast_node);

	ast_node->parent = ast_current;
	ast_node->line = parse_current->line;
	ast_node->hll_line = parse_current->hll_line;

	if (parse_current->data == CIL_KEY_BLOCK) {
		rc = cil_gen_block(db, parse_current, ast_node, 0);
	} else if (parse_current->data == CIL_KEY_BLOCKINHERIT) {
		rc = cil_gen_blockinherit(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_BLOCKABSTRACT) {
		rc = cil_gen_blockabstract(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_IN) {
		rc = cil_gen_in(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_CLASS) {
		rc = cil_gen_class(db, parse_current, ast_node);
		// To avoid parsing list of perms again
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CLASSORDER) {
		rc = cil_gen_classorder(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_MAP_CLASS) {
		rc = cil_gen_map_class(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CLASSMAPPING) {
		rc = cil_gen_classmapping(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CLASSPERMISSION) {
		rc = cil_gen_classpermission(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CLASSPERMISSIONSET) {
		rc = cil_gen_classpermissionset(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_COMMON) {
		rc = cil_gen_common(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CLASSCOMMON) {
		rc = cil_gen_classcommon(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_SID) {
		rc = cil_gen_sid(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SIDCONTEXT) {
		rc = cil_gen_sidcontext(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SIDORDER) {
		rc = cil_gen_sidorder(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_USER) {
		rc = cil_gen_user(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_USERATTRIBUTE) {
		rc = cil_gen_userattribute(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_USERATTRIBUTESET) {
		rc = cil_gen_userattributeset(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_USERLEVEL) {
		rc = cil_gen_userlevel(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_USERRANGE) {
		rc = cil_gen_userrange(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_USERBOUNDS) {
		rc = cil_gen_bounds(db, parse_current, ast_node, CIL_USER);
	} else if (parse_current->data == CIL_KEY_USERPREFIX) {
		rc = cil_gen_userprefix(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_SELINUXUSER) {
		rc = cil_gen_selinuxuser(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SELINUXUSERDEFAULT) {
		rc = cil_gen_selinuxuserdefault(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_TYPE) {
		rc = cil_gen_type(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_TYPEATTRIBUTE) {
		rc = cil_gen_typeattribute(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_TYPEATTRIBUTESET) {
		rc = cil_gen_typeattributeset(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_EXPANDTYPEATTRIBUTE) {
		rc = cil_gen_expandtypeattribute(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_TYPEALIAS) {
		rc = cil_gen_alias(db, parse_current, ast_node, CIL_TYPEALIAS);
	} else if (parse_current->data == CIL_KEY_TYPEALIASACTUAL) {
		rc = cil_gen_aliasactual(db, parse_current, ast_node, CIL_TYPEALIASACTUAL);
	} else if (parse_current->data == CIL_KEY_TYPEBOUNDS) {
		rc = cil_gen_bounds(db, parse_current, ast_node, CIL_TYPE);
	} else if (parse_current->data == CIL_KEY_TYPEPERMISSIVE) {
		rc = cil_gen_typepermissive(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_RANGETRANSITION) {
		rc = cil_gen_rangetransition(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_ROLE) {
		rc = cil_gen_role(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_USERROLE) {
		rc = cil_gen_userrole(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_ROLETYPE) {
		rc = cil_gen_roletype(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_ROLETRANSITION) {
		rc = cil_gen_roletransition(parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_ROLEALLOW) {
		rc = cil_gen_roleallow(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_ROLEATTRIBUTE) {
		rc = cil_gen_roleattribute(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_ROLEATTRIBUTESET) {
		rc = cil_gen_roleattributeset(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_ROLEBOUNDS) {
		rc = cil_gen_bounds(db, parse_current, ast_node, CIL_ROLE);
	} else if (parse_current->data == CIL_KEY_BOOL) {
		rc = cil_gen_bool(db, parse_current, ast_node, CIL_FALSE);
	} else if (parse_current->data == CIL_KEY_BOOLEANIF) {
		rc = cil_gen_boolif(db, parse_current, ast_node, CIL_FALSE);
	} else if(parse_current->data == CIL_KEY_TUNABLE) {
		if (db->preserve_tunables) {
			rc = cil_gen_bool(db, parse_current, ast_node, CIL_TRUE);
		} else {
			rc = cil_gen_tunable(db, parse_current, ast_node);
		}
	} else if (parse_current->data == CIL_KEY_TUNABLEIF) {
		if (db->preserve_tunables) {
			rc = cil_gen_boolif(db, parse_current, ast_node, CIL_TRUE);
		} else {
			rc = cil_gen_tunif(db, parse_current, ast_node);
		}
	} else if (parse_current->data == CIL_KEY_CONDTRUE) {
		rc = cil_gen_condblock(db, parse_current, ast_node, CIL_CONDTRUE);
	} else if (parse_current->data == CIL_KEY_CONDFALSE) {
		rc = cil_gen_condblock(db, parse_current, ast_node, CIL_CONDFALSE);
	} else if (parse_current->data == CIL_KEY_ALLOW) {
		rc = cil_gen_avrule(parse_current, ast_node, CIL_AVRULE_ALLOWED);
		// So that the object and perms lists do not get parsed again
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_AUDITALLOW) {
		rc = cil_gen_avrule(parse_current, ast_node, CIL_AVRULE_AUDITALLOW);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DONTAUDIT) {
		rc = cil_gen_avrule(parse_current, ast_node, CIL_AVRULE_DONTAUDIT);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_NEVERALLOW) {
		rc = cil_gen_avrule(parse_current, ast_node, CIL_AVRULE_NEVERALLOW);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_ALLOWX) {
		rc = cil_gen_avrulex(parse_current, ast_node, CIL_AVRULE_ALLOWED);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_AUDITALLOWX) {
		rc = cil_gen_avrulex(parse_current, ast_node, CIL_AVRULE_AUDITALLOW);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DONTAUDITX) {
		rc = cil_gen_avrulex(parse_current, ast_node, CIL_AVRULE_DONTAUDIT);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_NEVERALLOWX) {
		rc = cil_gen_avrulex(parse_current, ast_node, CIL_AVRULE_NEVERALLOW);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_PERMISSIONX) {
		rc = cil_gen_permissionx(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_TYPETRANSITION) {
		rc = cil_gen_typetransition(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_TYPECHANGE) {
		rc = cil_gen_type_rule(parse_current, ast_node, CIL_TYPE_CHANGE);
	} else if (parse_current->data == CIL_KEY_TYPEMEMBER) {
		rc = cil_gen_type_rule(parse_current, ast_node, CIL_TYPE_MEMBER);
	} else if (parse_current->data == CIL_KEY_SENSITIVITY) {
		rc = cil_gen_sensitivity(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_SENSALIAS) {
		rc = cil_gen_alias(db, parse_current, ast_node, CIL_SENSALIAS);
	} else if (parse_current->data == CIL_KEY_SENSALIASACTUAL) {
		rc = cil_gen_aliasactual(db, parse_current, ast_node, CIL_SENSALIASACTUAL);
	} else if (parse_current->data == CIL_KEY_CATEGORY) {
		rc = cil_gen_category(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_CATALIAS) {
		rc = cil_gen_alias(db, parse_current, ast_node, CIL_CATALIAS);
	} else if (parse_current->data == CIL_KEY_CATALIASACTUAL) {
		rc = cil_gen_aliasactual(db, parse_current, ast_node, CIL_CATALIASACTUAL);
	} else if (parse_current->data == CIL_KEY_CATSET) {
		rc = cil_gen_catset(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CATORDER) {
		rc = cil_gen_catorder(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SENSITIVITYORDER) {
		rc = cil_gen_sensitivityorder(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SENSCAT) {
		rc = cil_gen_senscat(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_LEVEL) {
		rc = cil_gen_level(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_LEVELRANGE) {
		rc = cil_gen_levelrange(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CONSTRAIN) {
		rc = cil_gen_constrain(db, parse_current, ast_node, CIL_CONSTRAIN);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_MLSCONSTRAIN) {
		rc = cil_gen_constrain(db, parse_current, ast_node, CIL_MLSCONSTRAIN);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_VALIDATETRANS) {
		rc = cil_gen_validatetrans(db, parse_current, ast_node, CIL_VALIDATETRANS);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_MLSVALIDATETRANS) {
		rc = cil_gen_validatetrans(db, parse_current, ast_node, CIL_MLSVALIDATETRANS);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_CONTEXT) {
		rc = cil_gen_context(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_FILECON) {
		rc = cil_gen_filecon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_IBPKEYCON) {
		rc = cil_gen_ibpkeycon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_IBENDPORTCON) {
		rc = cil_gen_ibendportcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_PORTCON) {
		rc = cil_gen_portcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_NODECON) {
		rc = cil_gen_nodecon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_GENFSCON) {
		rc = cil_gen_genfscon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_NETIFCON) {
		rc = cil_gen_netifcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_PIRQCON) {
		rc = cil_gen_pirqcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_IOMEMCON) {
		rc = cil_gen_iomemcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_IOPORTCON) {
		rc = cil_gen_ioportcon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_PCIDEVICECON) {
		rc = cil_gen_pcidevicecon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DEVICETREECON) {
		rc = cil_gen_devicetreecon(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_FSUSE) {
		rc = cil_gen_fsuse(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_MACRO) {
		rc = cil_gen_macro(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_CALL) {
		rc = cil_gen_call(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_POLICYCAP) {
		rc = cil_gen_policycap(db, parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_OPTIONAL) {
		rc = cil_gen_optional(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_IPADDR) {
		rc = cil_gen_ipaddr(db, parse_current, ast_node);
	} else if (parse_current->data == CIL_KEY_DEFAULTUSER) {
		rc = cil_gen_default(parse_current, ast_node, CIL_DEFAULTUSER);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DEFAULTROLE) {
		rc = cil_gen_default(parse_current, ast_node, CIL_DEFAULTROLE);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DEFAULTTYPE) {
		rc = cil_gen_default(parse_current, ast_node, CIL_DEFAULTTYPE);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_DEFAULTRANGE) {
		rc = cil_gen_defaultrange(parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_HANDLEUNKNOWN) {
		rc = cil_gen_handleunknown(parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_MLS) {
		rc = cil_gen_mls(parse_current, ast_node);
		*finished = CIL_TREE_SKIP_NEXT;
	} else if (parse_current->data == CIL_KEY_SRC_INFO) {
		rc = cil_gen_src_info(parse_current, ast_node);
	} else {
		cil_log(CIL_ERR, "Error: Unknown keyword %s\n", (char *)parse_current->data);
		rc = SEPOL_ERR;
	}

	if (rc == SEPOL_OK) {
		if (ast_current->cl_head == NULL) {
			ast_current->cl_head = ast_node;
		} else {
			ast_current->cl_tail->next = ast_node;
		}
		ast_current->cl_tail = ast_node;
		ast_current = ast_node;
		args->ast = ast_current;
	} else {
		cil_tree_node_destroy(&ast_node);
	}

exit:
	return rc;
}