static int samldb_user_account_control_change(struct samldb_ctx *ac)
{
	struct ldb_context *ldb = ldb_module_get_ctx(ac->module);
	uint32_t old_uac;
	uint32_t new_uac;
	uint32_t raw_uac;
	uint32_t old_ufa;
	uint32_t new_ufa;
	uint32_t old_uac_computed;
	uint32_t clear_uac;
	uint32_t old_atype;
	uint32_t new_atype;
	uint32_t old_pgrid;
	uint32_t new_pgrid;
	NTTIME old_lockoutTime;
	struct ldb_message_element *el;
	struct ldb_val *val;
	struct ldb_val computer_val;
	struct ldb_message *tmp_msg;
	struct dom_sid *sid;
	int ret;
	struct ldb_result *res;
	const char * const attrs[] = {
		"objectClass",
		"isCriticalSystemObject",
		"userAccountControl",
		"msDS-User-Account-Control-Computed",
		"lockoutTime",
		"objectSid",
		NULL
	};
	bool is_computer_objectclass = false;
	bool old_is_critical = false;
	bool new_is_critical = false;

	ret = dsdb_get_expected_new_values(ac,
					   ac->msg,
					   "userAccountControl",
					   &el,
					   ac->req->operation);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	if (el == NULL || el->num_values == 0) {
		ldb_asprintf_errstring(ldb,
			"%08X: samldb: 'userAccountControl' can't be deleted!",
			W_ERROR_V(WERR_DS_ILLEGAL_MOD_OPERATION));
		return LDB_ERR_UNWILLING_TO_PERFORM;
	}

	/* Create a temporary message for fetching the "userAccountControl" */
	tmp_msg = ldb_msg_new(ac->msg);
	if (tmp_msg == NULL) {
		return ldb_module_oom(ac->module);
	}
	ret = ldb_msg_add(tmp_msg, el, 0);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	raw_uac = ldb_msg_find_attr_as_uint(tmp_msg,
					    "userAccountControl",
					    0);
	talloc_free(tmp_msg);
	/*
	 * UF_LOCKOUT, UF_PASSWD_CANT_CHANGE and UF_PASSWORD_EXPIRED
	 * are only generated and not stored. We ignore them almost
	 * completely, along with unknown bits and UF_SCRIPT.
	 *
	 * The only exception is ACB_AUTOLOCK, which features in
	 * clear_acb when the bit is cleared in this modify operation.
	 *
	 * MS-SAMR 2.2.1.13 UF_FLAG Codes states that some bits are
	 * ignored by clients and servers
	 */
	new_uac = raw_uac & UF_SETTABLE_BITS;

	/* Fetch the old "userAccountControl" and "objectClass" */
	ret = dsdb_module_search_dn(ac->module, ac, &res, ac->msg->dn, attrs,
				    DSDB_FLAG_NEXT_MODULE, ac->req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	old_uac = ldb_msg_find_attr_as_uint(res->msgs[0], "userAccountControl", 0);
	if (old_uac == 0) {
		return ldb_operr(ldb);
	}
	old_uac_computed = ldb_msg_find_attr_as_uint(res->msgs[0],
						     "msDS-User-Account-Control-Computed", 0);
	old_lockoutTime = ldb_msg_find_attr_as_int64(res->msgs[0],
						     "lockoutTime", 0);
	old_is_critical = ldb_msg_find_attr_as_bool(res->msgs[0],
						    "isCriticalSystemObject", 0);
	/*
	 * When we do not have objectclass "computer" we cannot
	 * switch to a workstation or (RO)DC
	 */
	el = ldb_msg_find_element(res->msgs[0], "objectClass");
	if (el == NULL) {
		return ldb_operr(ldb);
	}
	computer_val = data_blob_string_const("computer");
	val = ldb_msg_find_val(el, &computer_val);
	if (val != NULL) {
		is_computer_objectclass = true;
	}

	old_ufa = old_uac & UF_ACCOUNT_TYPE_MASK;
	old_atype = ds_uf2atype(old_ufa);
	old_pgrid = ds_uf2prim_group_rid(old_uac);

	new_ufa = new_uac & UF_ACCOUNT_TYPE_MASK;
	if (new_ufa == 0) {
		/*
		 * "userAccountControl" = 0 or missing one of the
		 * types means "UF_NORMAL_ACCOUNT".  See MS-SAMR
		 * 3.1.1.8.10 point 8
		 */
		new_ufa = UF_NORMAL_ACCOUNT;
		new_uac |= new_ufa;
	}
	sid = samdb_result_dom_sid(res, res->msgs[0], "objectSid");
	if (sid == NULL) {
		return ldb_module_operr(ac->module);
	}

	ret = samldb_check_user_account_control_rules(ac, sid,
						      raw_uac,
						      new_uac,
						      old_uac,
						      is_computer_objectclass);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	new_atype = ds_uf2atype(new_ufa);
	new_pgrid = ds_uf2prim_group_rid(new_uac);

	clear_uac = (old_uac | old_uac_computed) & ~raw_uac;

	switch (new_ufa) {
	case UF_NORMAL_ACCOUNT:
		new_is_critical = old_is_critical;
		break;

	case UF_INTERDOMAIN_TRUST_ACCOUNT:
		new_is_critical = true;
		break;

	case UF_WORKSTATION_TRUST_ACCOUNT:
		new_is_critical = false;
		if (new_uac & UF_PARTIAL_SECRETS_ACCOUNT) {
			new_is_critical = true;
		}
		break;

	case UF_SERVER_TRUST_ACCOUNT:
		new_is_critical = true;
		break;

	default:
		ldb_asprintf_errstring(ldb,
			"%08X: samldb: invalid userAccountControl[0x%08X]",
			W_ERROR_V(WERR_INVALID_PARAMETER), raw_uac);
		return LDB_ERR_OTHER;
	}

	if (old_atype != new_atype) {
		ret = samdb_msg_add_uint(ldb, ac->msg, ac->msg,
					 "sAMAccountType", new_atype);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		el = ldb_msg_find_element(ac->msg, "sAMAccountType");
		el->flags = LDB_FLAG_MOD_REPLACE;
	}

	/* As per MS-SAMR 3.1.1.8.10 these flags have not to be set */
	if ((clear_uac & UF_LOCKOUT) && (old_lockoutTime != 0)) {
		/* "lockoutTime" reset as per MS-SAMR 3.1.1.8.10 */
		ldb_msg_remove_attr(ac->msg, "lockoutTime");
		ret = samdb_msg_add_uint64(ldb, ac->msg, ac->msg, "lockoutTime",
					   (NTTIME)0);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		el = ldb_msg_find_element(ac->msg, "lockoutTime");
		el->flags = LDB_FLAG_MOD_REPLACE;
	}

	/*
	 * "isCriticalSystemObject" might be set/changed
	 *
	 * Even a change from UF_NORMAL_ACCOUNT (implicitly FALSE) to
	 * UF_WORKSTATION_TRUST_ACCOUNT (actually FALSE) triggers
	 * creating the attribute.
	 */
	if (old_is_critical != new_is_critical || old_atype != new_atype) {
		ret = ldb_msg_add_string(ac->msg, "isCriticalSystemObject",
					 new_is_critical ? "TRUE": "FALSE");
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		el = ldb_msg_find_element(ac->msg,
					   "isCriticalSystemObject");
		el->flags = LDB_FLAG_MOD_REPLACE;
	}

	if (!ldb_msg_find_element(ac->msg, "primaryGroupID") &&
	    (old_pgrid != new_pgrid)) {
		/* Older AD deployments don't know about the RODC group */
		if (new_pgrid == DOMAIN_RID_READONLY_DCS) {
			ret = samldb_prim_group_tester(ac, new_pgrid);
			if (ret != LDB_SUCCESS) {
				return ret;
			}
		}

		ret = samdb_msg_add_uint(ldb, ac->msg, ac->msg,
					 "primaryGroupID", new_pgrid);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		el = ldb_msg_find_element(ac->msg,
					   "primaryGroupID");
		el->flags = LDB_FLAG_MOD_REPLACE;
	}

	/* Propagate eventual "userAccountControl" attribute changes */
	if (old_uac != new_uac) {
		char *tempstr = talloc_asprintf(ac->msg, "%d",
						new_uac);
		if (tempstr == NULL) {
			return ldb_module_oom(ac->module);
		}

		ret = ldb_msg_add_empty(ac->msg,
					"userAccountControl",
					LDB_FLAG_MOD_REPLACE,
					&el);
		el->values = talloc(ac->msg, struct ldb_val);
		el->num_values = 1;
		el->values[0].data = (uint8_t *) tempstr;
		el->values[0].length = strlen(tempstr);
	} else {
		ldb_msg_remove_attr(ac->msg, "userAccountControl");
	}

	return LDB_SUCCESS;
}