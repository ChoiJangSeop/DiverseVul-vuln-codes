static int rdn_name_modify(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	const struct ldb_val *rdn_val_p;
	struct ldb_message_element *e = NULL;
	struct ldb_control *recalculate_rdn_control = NULL;

	ldb = ldb_module_get_ctx(module);

	/* do not manipulate our control entries */
	if (ldb_dn_is_special(req->op.mod.message->dn)) {
		return ldb_next_request(module, req);
	}

	recalculate_rdn_control = ldb_request_get_control(req,
					LDB_CONTROL_RECALCULATE_RDN_OID);
	if (recalculate_rdn_control != NULL) {
		struct ldb_message *msg = NULL;
		const char *rdn_name = NULL;
		struct ldb_val rdn_val;
		const struct ldb_schema_attribute *a = NULL;
		struct ldb_request *mod_req = NULL;
		int ret;
		struct ldb_message_element *rdn_del = NULL;
		struct ldb_message_element *name_del = NULL;

		recalculate_rdn_control->critical = false;

		msg = ldb_msg_copy_shallow(req, req->op.mod.message);
		if (msg == NULL) {
			return ldb_module_oom(module);
		}

		/*
		 * The caller must pass a dummy 'name' attribute
		 * in order to bypass some high level checks.
		 *
		 * We just remove it and check nothing is left.
		 */
		ldb_msg_remove_attr(msg, "name");

		if (msg->num_elements != 0) {
			return ldb_module_operr(module);
		}

		rdn_name = ldb_dn_get_rdn_name(msg->dn);
		if (rdn_name == NULL) {
			return ldb_module_oom(module);
		}

		a = ldb_schema_attribute_by_name(ldb, rdn_name);
		if (a == NULL) {
			return ldb_module_operr(module);
		}

		if (a->name != NULL && strcmp(a->name, "*") != 0) {
			rdn_name = a->name;
		}

		rdn_val_p = ldb_dn_get_rdn_val(msg->dn);
		if (rdn_val_p == NULL) {
			return ldb_module_oom(module);
		}
		rdn_val = ldb_val_dup(msg, rdn_val_p);
		if (rdn_val.length == 0) {
			return ldb_module_oom(module);
		}

		/*
		 * This is a bit tricky:
		 *
		 * We want _DELETE elements (as "rdn_del" and "name_del" without
		 * values) first, followed by _ADD (with the real names)
		 * elements (with values). Then we fix up the "rdn_del" and
		 * "name_del" attributes.
		 */

		ret = ldb_msg_add_empty(msg, "rdn_del", LDB_FLAG_MOD_DELETE, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}
		ret = ldb_msg_add_empty(msg, rdn_name, LDB_FLAG_MOD_ADD, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}
		ret = ldb_msg_add_value(msg, rdn_name, &rdn_val, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}

		ret = ldb_msg_add_empty(msg, "name_del", LDB_FLAG_MOD_DELETE, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}
		ret = ldb_msg_add_empty(msg, "name", LDB_FLAG_MOD_ADD, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}
		ret = ldb_msg_add_value(msg, "name", &rdn_val, NULL);
		if (ret != 0) {
			return ldb_module_oom(module);
		}

		rdn_del = ldb_msg_find_element(msg, "rdn_del");
		if (rdn_del == NULL) {
			return ldb_module_operr(module);
		}
		rdn_del->name = talloc_strdup(msg->elements, rdn_name);
		if (rdn_del->name == NULL) {
			return ldb_module_oom(module);
		}
		name_del = ldb_msg_find_element(msg, "name_del");
		if (name_del == NULL) {
			return ldb_module_operr(module);
		}
		name_del->name = talloc_strdup(msg->elements, "name");
		if (name_del->name == NULL) {
			return ldb_module_oom(module);
		}

		ret = ldb_build_mod_req(&mod_req, ldb,
					req, msg, NULL,
					req, rdn_recalculate_callback,
					req);
		if (ret != LDB_SUCCESS) {
			return ldb_module_done(req, NULL, NULL, ret);
		}
		talloc_steal(mod_req, msg);

		ret = ldb_request_add_control(mod_req,
					      LDB_CONTROL_RECALCULATE_RDN_OID,
					      false, NULL);
		if (ret != LDB_SUCCESS) {
			return ldb_module_done(req, NULL, NULL, ret);
		}
		ret = ldb_request_add_control(mod_req,
					      LDB_CONTROL_PERMISSIVE_MODIFY_OID,
					      false, NULL);
		if (ret != LDB_SUCCESS) {
			return ldb_module_done(req, NULL, NULL, ret);
		}

		/* go on with the call chain */
		return ldb_next_request(module, mod_req);
	}

	rdn_val_p = ldb_dn_get_rdn_val(req->op.mod.message->dn);
	if (rdn_val_p == NULL) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	if (rdn_val_p->length == 0) {
		ldb_asprintf_errstring(ldb, "Empty RDN value on %s not permitted!",
				       ldb_dn_get_linearized(req->op.mod.message->dn));
		return LDB_ERR_INVALID_DN_SYNTAX;
	}

	e = ldb_msg_find_element(req->op.mod.message, "distinguishedName");
	if (e != NULL) {
		ldb_asprintf_errstring(ldb, "Modify of 'distinguishedName' on %s not permitted, must use 'rename' operation instead",
				       ldb_dn_get_linearized(req->op.mod.message->dn));
		if (LDB_FLAG_MOD_TYPE(e->flags) == LDB_FLAG_MOD_REPLACE) {
			return LDB_ERR_CONSTRAINT_VIOLATION;
		} else {
			return LDB_ERR_UNWILLING_TO_PERFORM;
		}
	}

	if (ldb_msg_find_element(req->op.mod.message, "name")) {
		ldb_asprintf_errstring(ldb, "Modify of 'name' on %s not permitted, must use 'rename' operation instead",
				       ldb_dn_get_linearized(req->op.mod.message->dn));
		return LDB_ERR_NOT_ALLOWED_ON_RDN;
	}

	if (ldb_msg_find_element(req->op.mod.message, ldb_dn_get_rdn_name(req->op.mod.message->dn))) {
		ldb_asprintf_errstring(ldb, "Modify of RDN '%s' on %s not permitted, must use 'rename' operation instead",
				       ldb_dn_get_rdn_name(req->op.mod.message->dn), ldb_dn_get_linearized(req->op.mod.message->dn));
		return LDB_ERR_NOT_ALLOWED_ON_RDN;
	}

	/* All OK, they kept their fingers out of the special attributes */
	return ldb_next_request(module, req);
}