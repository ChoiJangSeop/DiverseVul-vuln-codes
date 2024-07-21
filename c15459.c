static NTSTATUS idmap_sid_to_xid(struct idmap_context *idmap_ctx,
				 TALLOC_CTX *mem_ctx,
				 const struct dom_sid *sid,
				 struct unixid *unixid)
{
	int ret;
	NTSTATUS status;
	struct ldb_context *ldb = idmap_ctx->ldb_ctx;
	struct ldb_dn *dn;
	struct ldb_message *hwm_msg, *map_msg, *sam_msg;
	struct ldb_result *res = NULL;
	int trans = -1;
	uint32_t low, high, hwm, new_xid;
	struct dom_sid_buf sid_string;
	char *unixid_string, *hwm_string;
	bool hwm_entry_exists;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	const char *sam_attrs[] = {"uidNumber", "gidNumber", "samAccountType", NULL};

	if (sid_check_is_in_unix_users(sid)) {
		uint32_t rid;
		DEBUG(6, ("This is a local unix uid, just calculate that.\n"));
		status = dom_sid_split_rid(tmp_ctx, sid, NULL, &rid);
		if (!NT_STATUS_IS_OK(status)) {
			talloc_free(tmp_ctx);
			return status;
		}

		unixid->id = rid;
		unixid->type = ID_TYPE_UID;

		talloc_free(tmp_ctx);
		return NT_STATUS_OK;
	}

	if (sid_check_is_in_unix_groups(sid)) {
		uint32_t rid;
		DEBUG(6, ("This is a local unix gid, just calculate that.\n"));
		status = dom_sid_split_rid(tmp_ctx, sid, NULL, &rid);
		if (!NT_STATUS_IS_OK(status)) {
			talloc_free(tmp_ctx);
			return status;
		}

		unixid->id = rid;
		unixid->type = ID_TYPE_GID;

		talloc_free(tmp_ctx);
		return NT_STATUS_OK;
	}

	/* 
	 * First check against our local DB, to see if this user has a
	 * mapping there.  This means that the Samba4 AD DC behaves
	 * much like a winbindd member server running idmap_ad
	 */
	
	if (lpcfg_parm_bool(idmap_ctx->lp_ctx, NULL, "idmap_ldb", "use rfc2307", false)) {
		struct dom_sid_buf buf;
		ret = dsdb_search_one(idmap_ctx->samdb, tmp_ctx, &sam_msg,
				      ldb_get_default_basedn(idmap_ctx->samdb),
				      LDB_SCOPE_SUBTREE, sam_attrs, 0,
				      "(&(objectSid=%s)"
				      "(|(sAMaccountType=%u)(sAMaccountType=%u)(sAMaccountType=%u)"
				      "(sAMaccountType=%u)(sAMaccountType=%u))"
				      "(|(uidNumber=*)(gidNumber=*)))",
				      dom_sid_str_buf(sid, &buf),
				      ATYPE_ACCOUNT, ATYPE_WORKSTATION_TRUST, ATYPE_INTERDOMAIN_TRUST,
				      ATYPE_SECURITY_GLOBAL_GROUP, ATYPE_SECURITY_LOCAL_GROUP);
	} else {
		/* If we are not to use the rfc2307 attributes, we just emulate a non-match */
		ret = LDB_ERR_NO_SUCH_OBJECT;
	}

	if (ret == LDB_ERR_CONSTRAINT_VIOLATION) {
		struct dom_sid_buf buf;
		DEBUG(1, ("Search for objectSid=%s gave duplicate results, failing to map to a unix ID!\n",
			  dom_sid_str_buf(sid, &buf)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	} else if (ret == LDB_SUCCESS) {
		uint32_t account_type = ldb_msg_find_attr_as_uint(sam_msg, "sAMaccountType", 0);
		if ((account_type == ATYPE_ACCOUNT) ||
		    (account_type == ATYPE_WORKSTATION_TRUST ) ||
		    (account_type == ATYPE_INTERDOMAIN_TRUST ))
		{
			const struct ldb_val *v = ldb_msg_find_ldb_val(sam_msg, "uidNumber");
			if (v) {
				unixid->type = ID_TYPE_UID;
				unixid->id = ldb_msg_find_attr_as_uint(sam_msg, "uidNumber", -1);
				talloc_free(tmp_ctx);
				return NT_STATUS_OK;
			}

		} else if ((account_type == ATYPE_SECURITY_GLOBAL_GROUP) ||
			   (account_type == ATYPE_SECURITY_LOCAL_GROUP))
		{
			const struct ldb_val *v = ldb_msg_find_ldb_val(sam_msg, "gidNumber");
			if (v) {
				unixid->type = ID_TYPE_GID;
				unixid->id = ldb_msg_find_attr_as_uint(sam_msg, "gidNumber", -1);
				talloc_free(tmp_ctx);
				return NT_STATUS_OK;
			}
		}
	} else if (ret != LDB_ERR_NO_SUCH_OBJECT) {
		struct dom_sid_buf buf;
		DEBUG(1, ("Search for objectSid=%s gave '%s', failing to map to a SID!\n",
			  dom_sid_str_buf(sid, &buf),
			  ldb_errstring(idmap_ctx->samdb)));

		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = ldb_search(ldb, tmp_ctx, &res, NULL, LDB_SCOPE_SUBTREE,
				 NULL, "(&(objectClass=sidMap)(objectSid=%s))",
				 ldap_encode_ndr_dom_sid(tmp_ctx, sid));
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Search failed: %s\n", ldb_errstring(ldb)));
		talloc_free(tmp_ctx);
		return NT_STATUS_NONE_MAPPED;
	}

	if (res->count == 1) {
		const char *type = ldb_msg_find_attr_as_string(res->msgs[0],
							       "type", NULL);
		new_xid = ldb_msg_find_attr_as_uint(res->msgs[0], "xidNumber",
						    -1);
		if (new_xid == (uint32_t) -1) {
			DEBUG(1, ("Invalid xid mapping.\n"));
			talloc_free(tmp_ctx);
			return NT_STATUS_NONE_MAPPED;
		}

		if (type == NULL) {
			DEBUG(1, ("Invalid type for mapping entry.\n"));
			talloc_free(tmp_ctx);
			return NT_STATUS_NONE_MAPPED;
		}

		unixid->id = new_xid;

		if (strcmp(type, "ID_TYPE_BOTH") == 0) {
			unixid->type = ID_TYPE_BOTH;
		} else if (strcmp(type, "ID_TYPE_UID") == 0) {
			unixid->type = ID_TYPE_UID;
		} else {
			unixid->type = ID_TYPE_GID;
		}

		talloc_free(tmp_ctx);
		return NT_STATUS_OK;
	}

	DEBUG(6, ("No existing mapping found, attempting to create one.\n"));

	trans = ldb_transaction_start(ldb);
	if (trans != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	/* Redo the search to make sure no one changed the mapping while we
	 * weren't looking */
	ret = ldb_search(ldb, tmp_ctx, &res, NULL, LDB_SCOPE_SUBTREE,
				 NULL, "(&(objectClass=sidMap)(objectSid=%s))",
				 ldap_encode_ndr_dom_sid(tmp_ctx, sid));
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Search failed: %s\n", ldb_errstring(ldb)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	if (res->count > 0) {
		DEBUG(1, ("Database changed while trying to add a sidmap.\n"));
		status = NT_STATUS_RETRY;
		goto failed;
	}

	ret = idmap_get_bounds(idmap_ctx, &low, &high);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	dn = ldb_dn_new(tmp_ctx, ldb, "CN=CONFIG");
	if (dn == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	ret = ldb_search(ldb, tmp_ctx, &res, dn, LDB_SCOPE_BASE, NULL, NULL);
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Search failed: %s\n", ldb_errstring(ldb)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	if (res->count != 1) {
		DEBUG(1, ("No CN=CONFIG record, idmap database is broken.\n"));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	hwm = ldb_msg_find_attr_as_uint(res->msgs[0], "xidNumber", -1);
	if (hwm == (uint32_t)-1) {
		hwm = low;
		hwm_entry_exists = false;
	} else {
		hwm_entry_exists = true;
	}

	if (hwm > high) {
		DEBUG(1, ("Out of xids to allocate.\n"));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	hwm_msg = ldb_msg_new(tmp_ctx);
	if (hwm_msg == NULL) {
		DEBUG(1, ("Out of memory when creating ldb_message\n"));
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	hwm_msg->dn = dn;

	new_xid = hwm;
	hwm++;

	hwm_string = talloc_asprintf(tmp_ctx, "%u", hwm);
	if (hwm_string == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	dom_sid_str_buf(sid, &sid_string);

	unixid_string = talloc_asprintf(tmp_ctx, "%u", new_xid);
	if (unixid_string == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	if (hwm_entry_exists) {
		struct ldb_message_element *els;
		struct ldb_val *vals;

		/* We're modifying the entry, not just adding a new one. */
		els = talloc_array(tmp_ctx, struct ldb_message_element, 2);
		if (els == NULL) {
			status = NT_STATUS_NO_MEMORY;
			goto failed;
		}

		vals = talloc_array(tmp_ctx, struct ldb_val, 2);
		if (vals == NULL) {
			status = NT_STATUS_NO_MEMORY;
			goto failed;
		}

		hwm_msg->num_elements = 2;
		hwm_msg->elements = els;

		els[0].num_values = 1;
		els[0].values = &vals[0];
		els[0].flags = LDB_FLAG_MOD_DELETE;
		els[0].name = talloc_strdup(tmp_ctx, "xidNumber");
		if (els[0].name == NULL) {
			status = NT_STATUS_NO_MEMORY;
			goto failed;
		}

		els[1].num_values = 1;
		els[1].values = &vals[1];
		els[1].flags = LDB_FLAG_MOD_ADD;
		els[1].name = els[0].name;

		vals[0].data = (uint8_t *)unixid_string;
		vals[0].length = strlen(unixid_string);
		vals[1].data = (uint8_t *)hwm_string;
		vals[1].length = strlen(hwm_string);
	} else {
		ret = ldb_msg_add_empty(hwm_msg, "xidNumber", LDB_FLAG_MOD_ADD,
					NULL);
		if (ret != LDB_SUCCESS) {
			status = NT_STATUS_NONE_MAPPED;
			goto failed;
		}

		ret = ldb_msg_add_string(hwm_msg, "xidNumber", hwm_string);
		if (ret != LDB_SUCCESS)
		{
			status = NT_STATUS_NONE_MAPPED;
			goto failed;
		}
	}

	ret = ldb_modify(ldb, hwm_msg);
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Updating the xid high water mark failed: %s\n",
			  ldb_errstring(ldb)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	map_msg = ldb_msg_new(tmp_ctx);
	if (map_msg == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	map_msg->dn = ldb_dn_new_fmt(tmp_ctx, ldb, "CN=%s", sid_string.buf);
	if (map_msg->dn == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	ret = ldb_msg_add_string(map_msg, "xidNumber", unixid_string);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = idmap_msg_add_dom_sid(idmap_ctx, tmp_ctx, map_msg, "objectSid",
			sid);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = ldb_msg_add_string(map_msg, "objectClass", "sidMap");
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = ldb_msg_add_string(map_msg, "type", "ID_TYPE_BOTH");
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = ldb_msg_add_string(map_msg, "cn", sid_string.buf);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	ret = ldb_add(ldb, map_msg);
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Adding a sidmap failed: %s\n", ldb_errstring(ldb)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	trans = ldb_transaction_commit(ldb);
	if (trans != LDB_SUCCESS) {
		DEBUG(1, ("Transaction failed: %s\n", ldb_errstring(ldb)));
		status = NT_STATUS_NONE_MAPPED;
		goto failed;
	}

	unixid->id = new_xid;
	unixid->type = ID_TYPE_BOTH;
	talloc_free(tmp_ctx);
	return NT_STATUS_OK;

failed:
	if (trans == LDB_SUCCESS) ldb_transaction_cancel(ldb);
	talloc_free(tmp_ctx);
	return status;
}