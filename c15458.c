static int descriptor_modify(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	struct ldb_request *mod_req;
	struct ldb_message *msg;
	struct ldb_result *current_res, *parent_res;
	const struct ldb_val *old_sd = NULL;
	const struct ldb_val *parent_sd = NULL;
	const struct ldb_val *user_sd;
	struct ldb_dn *dn = req->op.mod.message->dn;
	struct ldb_dn *parent_dn;
	struct ldb_message_element *objectclass_element, *sd_element;
	int ret;
	uint32_t instanceType;
	bool explicit_sd_flags = false;
	uint32_t sd_flags = dsdb_request_sd_flags(req, &explicit_sd_flags);
	const struct dsdb_schema *schema;
	DATA_BLOB *sd;
	const struct dsdb_class *objectclass;
	static const char * const parent_attrs[] = { "nTSecurityDescriptor", NULL };
	static const char * const current_attrs[] = { "nTSecurityDescriptor",
						      "instanceType",
						      "objectClass", NULL };
	struct GUID parent_guid = { .time_low = 0 };
	struct ldb_control *sd_propagation_control;
	int cmp_ret = -1;

	/* do not manipulate our control entries */
	if (ldb_dn_is_special(dn)) {
		return ldb_next_request(module, req);
	}

	sd_propagation_control = ldb_request_get_control(req,
					DSDB_CONTROL_SEC_DESC_PROPAGATION_OID);
	if (sd_propagation_control != NULL) {
		if (sd_propagation_control->data != module) {
			return ldb_operr(ldb);
		}
		if (req->op.mod.message->num_elements != 0) {
			return ldb_operr(ldb);
		}
		if (explicit_sd_flags) {
			return ldb_operr(ldb);
		}
		if (sd_flags != 0xF) {
			return ldb_operr(ldb);
		}
		if (sd_propagation_control->critical == 0) {
			return ldb_operr(ldb);
		}

		sd_propagation_control->critical = 0;
	}

	sd_element = ldb_msg_find_element(req->op.mod.message, "nTSecurityDescriptor");
	if (sd_propagation_control == NULL && sd_element == NULL) {
		return ldb_next_request(module, req);
	}

	/*
	 * nTSecurityDescriptor with DELETE is not supported yet.
	 * TODO: handle this correctly.
	 */
	if (sd_propagation_control == NULL &&
	    LDB_FLAG_MOD_TYPE(sd_element->flags) == LDB_FLAG_MOD_DELETE)
	{
		return ldb_module_error(module,
					LDB_ERR_UNWILLING_TO_PERFORM,
					"MOD_DELETE for nTSecurityDescriptor "
					"not supported yet");
	}

	user_sd = ldb_msg_find_ldb_val(req->op.mod.message, "nTSecurityDescriptor");
	/* nTSecurityDescriptor without a value is an error, letting through so it is handled */
	if (sd_propagation_control == NULL && user_sd == NULL) {
		return ldb_next_request(module, req);
	}

	ldb_debug(ldb, LDB_DEBUG_TRACE,"descriptor_modify: %s\n", ldb_dn_get_linearized(dn));

	ret = dsdb_module_search_dn(module, req, &current_res, dn,
				    current_attrs,
				    DSDB_FLAG_NEXT_MODULE |
				    DSDB_FLAG_AS_SYSTEM |
				    DSDB_SEARCH_SHOW_RECYCLED |
				    DSDB_SEARCH_SHOW_EXTENDED_DN,
				    req);
	if (ret != LDB_SUCCESS) {
		ldb_debug(ldb, LDB_DEBUG_ERROR,"descriptor_modify: Could not find %s\n",
			  ldb_dn_get_linearized(dn));
		return ret;
	}

	instanceType = ldb_msg_find_attr_as_uint(current_res->msgs[0],
						 "instanceType", 0);
	/* if the object has a parent, retrieve its SD to
	 * use for calculation */
	if (!ldb_dn_is_null(current_res->msgs[0]->dn) &&
	    !(instanceType & INSTANCE_TYPE_IS_NC_HEAD)) {
		NTSTATUS status;

		parent_dn = ldb_dn_get_parent(req, dn);
		if (parent_dn == NULL) {
			return ldb_oom(ldb);
		}
		ret = dsdb_module_search_dn(module, req, &parent_res, parent_dn,
					    parent_attrs,
					    DSDB_FLAG_NEXT_MODULE |
					    DSDB_FLAG_AS_SYSTEM |
					    DSDB_SEARCH_SHOW_RECYCLED |
					    DSDB_SEARCH_SHOW_EXTENDED_DN,
					    req);
		if (ret != LDB_SUCCESS) {
			ldb_debug(ldb, LDB_DEBUG_ERROR, "descriptor_modify: Could not find SD for %s\n",
				  ldb_dn_get_linearized(parent_dn));
			return ret;
		}
		if (parent_res->count != 1) {
			return ldb_operr(ldb);
		}
		parent_sd = ldb_msg_find_ldb_val(parent_res->msgs[0], "nTSecurityDescriptor");

		status = dsdb_get_extended_dn_guid(parent_res->msgs[0]->dn,
						   &parent_guid,
						   "GUID");
		if (!NT_STATUS_IS_OK(status)) {
			return ldb_operr(ldb);
		}
	}

	schema = dsdb_get_schema(ldb, req);

	objectclass_element = ldb_msg_find_element(current_res->msgs[0], "objectClass");
	if (objectclass_element == NULL) {
		return ldb_operr(ldb);
	}

	objectclass = dsdb_get_last_structural_class(schema,
						     objectclass_element);
	if (objectclass == NULL) {
		return ldb_operr(ldb);
	}

	old_sd = ldb_msg_find_ldb_val(current_res->msgs[0], "nTSecurityDescriptor");
	if (old_sd == NULL) {
		return ldb_operr(ldb);
	}

	if (sd_propagation_control != NULL) {
		/*
		 * This just triggers a recalculation of the
		 * inherited aces.
		 */
		user_sd = old_sd;
	}

	sd = get_new_descriptor(module, current_res->msgs[0]->dn, req,
				objectclass, parent_sd,
				user_sd, old_sd, sd_flags);
	if (sd == NULL) {
		return ldb_operr(ldb);
	}
	msg = ldb_msg_copy_shallow(req, req->op.mod.message);
	if (msg == NULL) {
		return ldb_oom(ldb);
	}
	cmp_ret = data_blob_cmp(old_sd, sd);
	if (sd_propagation_control != NULL) {
		if (cmp_ret == 0) {
			/*
			 * The nTSecurityDescriptor is unchanged,
			 * which means we can stop the processing.
			 *
			 * We mark the control as critical again,
			 * as we have not processed it, so the caller
			 * can tell that the descriptor was unchanged.
			 */
			sd_propagation_control->critical = 1;
			return ldb_module_done(req, NULL, NULL, LDB_SUCCESS);
		}

		ret = ldb_msg_add_empty(msg, "nTSecurityDescriptor",
					LDB_FLAG_MOD_REPLACE,
					&sd_element);
		if (ret != LDB_SUCCESS) {
			return ldb_oom(ldb);
		}
		ret = ldb_msg_add_value(msg, "nTSecurityDescriptor",
					sd, NULL);
		if (ret != LDB_SUCCESS) {
			return ldb_oom(ldb);
		}
	} else if (cmp_ret != 0) {
		struct GUID guid;
		struct ldb_dn *nc_root;
		NTSTATUS status;

		ret = dsdb_find_nc_root(ldb,
					msg,
					current_res->msgs[0]->dn,
					&nc_root);
		if (ret != LDB_SUCCESS) {
			return ldb_oom(ldb);
		}

		status = dsdb_get_extended_dn_guid(current_res->msgs[0]->dn,
						   &guid,
						   "GUID");
		if (!NT_STATUS_IS_OK(status)) {
			return ldb_operr(ldb);
		}

		/*
		 * Force SD propagation on children of this record
		 */
		ret = dsdb_module_schedule_sd_propagation(module,
							  nc_root,
							  guid,
							  parent_guid,
							  false);
		if (ret != LDB_SUCCESS) {
			return ldb_operr(ldb);
		}
		sd_element->values[0] = *sd;
	} else {
		sd_element->values[0] = *sd;
	}

	ret = ldb_build_mod_req(&mod_req, ldb, req,
				msg,
				req->controls,
				req,
				dsdb_next_callback,
				req);
	LDB_REQ_SET_LOCATION(mod_req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	return ldb_next_request(module, mod_req);
}