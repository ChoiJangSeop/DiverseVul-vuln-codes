static int samldb_group_type_change(struct samldb_ctx *ac)
{
	struct ldb_context *ldb = ldb_module_get_ctx(ac->module);
	uint32_t group_type, old_group_type, account_type;
	struct ldb_message_element *el;
	struct ldb_message *tmp_msg;
	int ret;
	struct ldb_result *res;
	const char * const attrs[] = { "groupType", NULL };

	ret = dsdb_get_expected_new_values(ac,
					   ac->msg,
					   "groupType",
					   &el,
					   ac->req->operation);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	if (el == NULL) {
		/* we are not affected */
		return LDB_SUCCESS;
	}

	/* Create a temporary message for fetching the "groupType" */
	tmp_msg = ldb_msg_new(ac->msg);
	if (tmp_msg == NULL) {
		return ldb_module_oom(ac->module);
	}
	ret = ldb_msg_add(tmp_msg, el, 0);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	group_type = ldb_msg_find_attr_as_uint(tmp_msg, "groupType", 0);
	talloc_free(tmp_msg);

	ret = dsdb_module_search_dn(ac->module, ac, &res, ac->msg->dn, attrs,
				    DSDB_FLAG_NEXT_MODULE |
				    DSDB_SEARCH_SHOW_DELETED, ac->req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	old_group_type = ldb_msg_find_attr_as_uint(res->msgs[0], "groupType", 0);
	if (old_group_type == 0) {
		return ldb_operr(ldb);
	}

	/* Group type switching isn't so easy as it seems: We can only
	 * change in this directions: global <-> universal <-> local
	 * On each step also the group type itself
	 * (security/distribution) is variable. */

	if (ldb_request_get_control(ac->req, LDB_CONTROL_PROVISION_OID) == NULL) {
		switch (group_type) {
		case GTYPE_SECURITY_GLOBAL_GROUP:
		case GTYPE_DISTRIBUTION_GLOBAL_GROUP:
			/* change to "universal" allowed */
			if ((old_group_type == GTYPE_SECURITY_DOMAIN_LOCAL_GROUP) ||
			(old_group_type == GTYPE_DISTRIBUTION_DOMAIN_LOCAL_GROUP)) {
				ldb_set_errstring(ldb,
					"samldb: Change from security/distribution local group forbidden!");
				return LDB_ERR_UNWILLING_TO_PERFORM;
			}
		break;

		case GTYPE_SECURITY_UNIVERSAL_GROUP:
		case GTYPE_DISTRIBUTION_UNIVERSAL_GROUP:
			/* each change allowed */
		break;
		case GTYPE_SECURITY_DOMAIN_LOCAL_GROUP:
		case GTYPE_DISTRIBUTION_DOMAIN_LOCAL_GROUP:
			/* change to "universal" allowed */
			if ((old_group_type == GTYPE_SECURITY_GLOBAL_GROUP) ||
			(old_group_type == GTYPE_DISTRIBUTION_GLOBAL_GROUP)) {
				ldb_set_errstring(ldb,
					"samldb: Change from security/distribution global group forbidden!");
				return LDB_ERR_UNWILLING_TO_PERFORM;
			}
		break;

		case GTYPE_SECURITY_BUILTIN_LOCAL_GROUP:
		default:
			/* we don't allow this "groupType" values */
			return LDB_ERR_UNWILLING_TO_PERFORM;
		break;
		}
	}

	account_type =  ds_gtype2atype(group_type);
	if (account_type == 0) {
		ldb_set_errstring(ldb, "samldb: Unrecognized account type!");
		return LDB_ERR_UNWILLING_TO_PERFORM;
	}
	ret = samdb_msg_add_uint(ldb, ac->msg, ac->msg, "sAMAccountType",
				 account_type);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	el = ldb_msg_find_element(ac->msg, "sAMAccountType");
	el->flags = LDB_FLAG_MOD_REPLACE;

	return LDB_SUCCESS;
}