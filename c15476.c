static int tr_prepare_attributes(struct tr_context *ac)
{
	struct ldb_context *ldb = ldb_module_get_ctx(ac->module);
	int ret;
	struct ldb_message_element *el = NULL;
	uint32_t account_type, user_account_control;
	struct ldb_dn *objectcategory = NULL;

	ac->mod_msg = ldb_msg_copy_shallow(ac, ac->req_msg);
	if (ac->mod_msg == NULL) {
		return ldb_oom(ldb);
	}

	ac->mod_res = talloc_zero(ac, struct ldb_result);
	if (ac->mod_res == NULL) {
		return ldb_oom(ldb);
	}

	ret = ldb_build_mod_req(&ac->mod_req, ldb, ac,
				ac->mod_msg,
				NULL,
				ac->mod_res,
				ldb_modify_default_callback,
				ac->req);
	LDB_REQ_SET_LOCATION(ac->mod_req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	/* - remove distinguishedName - we don't need it */
	ldb_msg_remove_attr(ac->mod_msg, "distinguishedName");

	/* remove isRecycled */
	ret = ldb_msg_add_empty(ac->mod_msg, "isRecycled",
				LDB_FLAG_MOD_DELETE, NULL);
	if (ret != LDB_SUCCESS) {
		ldb_asprintf_errstring(ldb, "Failed to reset isRecycled attribute: %s", ldb_strerror(ret));
		return LDB_ERR_OPERATIONS_ERROR;
	}

	/* objectClass is USER */
	if (samdb_find_attribute(ldb, ac->search_msg, "objectclass", "user") != NULL) {
		uint32_t primary_group_rid;
		/* restoring 'user' instance attribute is heavily borrowed from samldb.c */

		/* Default values */
		ret = dsdb_user_obj_set_defaults(ldb, ac->mod_msg, ac->mod_req);
		if (ret != LDB_SUCCESS) return ret;

		/* "userAccountControl" must exists on deleted object */
		user_account_control = ldb_msg_find_attr_as_uint(ac->search_msg,
							"userAccountControl",
							(uint32_t)-1);
		if (user_account_control == (uint32_t)-1) {
			return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
					 "reanimate: No 'userAccountControl' attribute found!");
		}

		/* restore "sAMAccountType" */
		ret = dsdb_user_obj_set_account_type(ldb, ac->mod_msg,
						     user_account_control, NULL);
		if (ret != LDB_SUCCESS) {
			return ret;
		}

		/* "userAccountControl" -> "primaryGroupID" mapping */
		ret = dsdb_user_obj_set_primary_group_id(ldb, ac->mod_msg,
							 user_account_control,
							 &primary_group_rid);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		/*
		 * Older AD deployments don't know about the
		 * RODC group
		 */
		if (primary_group_rid == DOMAIN_RID_READONLY_DCS) {
			/* TODO:  check group exists */
		}

	}

	/* objectClass is GROUP */
	if (samdb_find_attribute(ldb, ac->search_msg, "objectclass", "group") != NULL) {
		/* "groupType" -> "sAMAccountType" */
		uint32_t group_type;

		el = ldb_msg_find_element(ac->search_msg, "groupType");
		if (el == NULL) {
			return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
					 "reanimate: Unexpected: missing groupType attribute.");
		}

		group_type = ldb_msg_find_attr_as_uint(ac->search_msg,
						       "groupType", 0);

		account_type = ds_gtype2atype(group_type);
		if (account_type == 0) {
			return ldb_error(ldb, LDB_ERR_UNWILLING_TO_PERFORM,
					 "reanimate: Unrecognized account type!");
		}
		ret = samdb_msg_add_uint(ldb, ac->mod_msg, ac->mod_msg,
					 "sAMAccountType", account_type);
		if (ret != LDB_SUCCESS) {
			return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
					 "reanimate: Failed to add sAMAccountType to restored object.");
		}
		el = ldb_msg_find_element(ac->mod_msg, "sAMAccountType");
		el->flags = LDB_FLAG_MOD_REPLACE;

		/* Default values set by Windows */
		ret = samdb_find_or_add_attribute(ldb, ac->mod_msg,
						  "adminCount", "0");
		if (ret != LDB_SUCCESS) return ret;
		ret = samdb_find_or_add_attribute(ldb, ac->mod_msg,
						  "operatorCount", "0");
		if (ret != LDB_SUCCESS) return ret;
	}

	/* - restore objectCategory if not present */
	objectcategory = ldb_msg_find_attr_as_dn(ldb, ac, ac->search_msg,
						 "objectCategory");
	if (objectcategory == NULL) {
		const char *value;

		ret = dsdb_make_object_category(ldb, ac->schema, ac->search_msg,
						ac->mod_msg, &value);
		if (ret != LDB_SUCCESS) {
			return ret;
		}

		ret = ldb_msg_add_string(ac->mod_msg, "objectCategory", value);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		el = ldb_msg_find_element(ac->mod_msg, "objectCategory");
		el->flags = LDB_FLAG_MOD_ADD;
	}

	return LDB_SUCCESS;
}