static NTSTATUS update_trust_user(TALLOC_CTX *mem_ctx,
				  struct ldb_context *sam_ldb,
				  struct ldb_dn *base_dn,
				  bool delete_user,
				  const char *netbios_name,
				  struct trustAuthInOutBlob *in)
{
	const char *attrs[] = { "userAccountControl", NULL };
	struct ldb_message **msgs;
	struct ldb_message *msg;
	uint32_t uac;
	uint32_t i;
	int ret;

	ret = gendb_search(sam_ldb, mem_ctx,
			   base_dn, &msgs, attrs,
			   "samAccountName=%s$", netbios_name);
	if (ret > 1) {
		return NT_STATUS_INTERNAL_DB_CORRUPTION;
	}

	if (ret == 0) {
		if (delete_user) {
			return NT_STATUS_OK;
		}

		/* ok no existing user, add it from scratch */
		return add_trust_user(mem_ctx, sam_ldb, base_dn,
				      netbios_name, in, NULL);
	}

	/* check user is what we are looking for */
	uac = ldb_msg_find_attr_as_uint(msgs[0],
					"userAccountControl", 0);
	if (!(uac & UF_INTERDOMAIN_TRUST_ACCOUNT)) {
		return NT_STATUS_OBJECT_NAME_COLLISION;
	}

	if (delete_user) {
		ret = ldb_delete(sam_ldb, msgs[0]->dn);
		switch (ret) {
		case LDB_SUCCESS:
			return NT_STATUS_OK;
		case LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS:
			return NT_STATUS_ACCESS_DENIED;
		default:
			return NT_STATUS_INTERNAL_DB_CORRUPTION;
		}
	}

	/* entry exists, just modify secret if any */
	if (in == NULL || in->count == 0) {
		return NT_STATUS_OK;
	}

	msg = ldb_msg_new(mem_ctx);
	if (!msg) {
		return NT_STATUS_NO_MEMORY;
	}
	msg->dn = msgs[0]->dn;

	for (i = 0; i < in->count; i++) {
		const char *attribute;
		struct ldb_val v;
		switch (in->current.array[i].AuthType) {
		case TRUST_AUTH_TYPE_NT4OWF:
			attribute = "unicodePwd";
			v.data = (uint8_t *)&in->current.array[i].AuthInfo.nt4owf.password;
			v.length = 16;
			break;
		case TRUST_AUTH_TYPE_CLEAR:
			attribute = "clearTextPassword";
			v.data = in->current.array[i].AuthInfo.clear.password;
			v.length = in->current.array[i].AuthInfo.clear.size;
			break;
		default:
			continue;
		}

		ret = ldb_msg_add_empty(msg, attribute,
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_NO_MEMORY;
		}

		ret = ldb_msg_add_value(msg, attribute, &v, NULL);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_NO_MEMORY;
		}
	}

	/* create the trusted_domain user account */
	ret = ldb_modify(sam_ldb, msg);
	if (ret != LDB_SUCCESS) {
		DEBUG(0,("Failed to create user record %s: %s\n",
			 ldb_dn_get_linearized(msg->dn),
			 ldb_errstring(sam_ldb)));

		switch (ret) {
		case LDB_ERR_ENTRY_ALREADY_EXISTS:
			return NT_STATUS_DOMAIN_EXISTS;
		case LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS:
			return NT_STATUS_ACCESS_DENIED;
		default:
			return NT_STATUS_INTERNAL_DB_CORRUPTION;
		}
	}

	return NT_STATUS_OK;
}