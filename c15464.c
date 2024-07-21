NTSTATUS samdb_set_password_sid(struct ldb_context *ldb, TALLOC_CTX *mem_ctx,
				const struct dom_sid *user_sid,
				const uint32_t *new_version, /* optional for trusts */
				const DATA_BLOB *new_password,
				const struct samr_Password *ntNewHash,
				enum dsdb_password_checked old_password_checked,
				enum samPwdChangeReason *reject_reason,
				struct samr_DomInfo1 **_dominfo) 
{
	TALLOC_CTX *frame = talloc_stackframe();
	NTSTATUS nt_status;
	const char * const user_attrs[] = {
		"userAccountControl",
		"sAMAccountName",
		NULL
	};
	struct ldb_message *user_msg = NULL;
	int ret;
	uint32_t uac = 0;

	ret = ldb_transaction_start(ldb);
	if (ret != LDB_SUCCESS) {
		DEBUG(1, ("Failed to start transaction: %s\n", ldb_errstring(ldb)));
		TALLOC_FREE(frame);
		return NT_STATUS_TRANSACTION_ABORTED;
	}

	ret = dsdb_search_one(ldb, frame, &user_msg, ldb_get_default_basedn(ldb),
			      LDB_SCOPE_SUBTREE, user_attrs, 0,
			      "(&(objectSid=%s)(objectClass=user))",
			      ldap_encode_ndr_dom_sid(frame, user_sid));
	if (ret != LDB_SUCCESS) {
		ldb_transaction_cancel(ldb);
		DEBUG(3, ("samdb_set_password_sid: SID[%s] not found in samdb %s - %s, "
			  "returning NO_SUCH_USER\n",
			  dom_sid_string(frame, user_sid),
			  ldb_strerror(ret), ldb_errstring(ldb)));
		TALLOC_FREE(frame);
		return NT_STATUS_NO_SUCH_USER;
	}

	uac = ldb_msg_find_attr_as_uint(user_msg, "userAccountControl", 0);
	if (!(uac & UF_ACCOUNT_TYPE_MASK)) {
		ldb_transaction_cancel(ldb);
		DEBUG(1, ("samdb_set_password_sid: invalid "
			  "userAccountControl[0x%08X] for SID[%s] DN[%s], "
			  "returning NO_SUCH_USER\n",
			  (unsigned)uac, dom_sid_string(frame, user_sid),
			  ldb_dn_get_linearized(user_msg->dn)));
		TALLOC_FREE(frame);
		return NT_STATUS_NO_SUCH_USER;
	}

	if (uac & UF_INTERDOMAIN_TRUST_ACCOUNT) {
		const char * const tdo_attrs[] = {
			"trustAuthIncoming",
			"trustDirection",
			NULL
		};
		struct ldb_message *tdo_msg = NULL;
		const char *account_name = NULL;
		uint32_t trust_direction;
		uint32_t i;
		const struct ldb_val *old_val = NULL;
		struct trustAuthInOutBlob old_blob = {
			.count = 0,
		};
		uint32_t old_version = 0;
		struct AuthenticationInformation *old_version_a = NULL;
		uint32_t _new_version = 0;
		struct trustAuthInOutBlob new_blob = {
			.count = 0,
		};
		struct ldb_val new_val = {
			.length = 0,
		};
		struct timeval tv = timeval_current();
		NTTIME now = timeval_to_nttime(&tv);
		enum ndr_err_code ndr_err;

		if (new_password == NULL && ntNewHash == NULL) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: "
				  "no new password provided "
				  "sAMAccountName for SID[%s] DN[%s], "
				  "returning INVALID_PARAMETER\n",
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (new_password != NULL && ntNewHash != NULL) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: "
				  "two new passwords provided "
				  "sAMAccountName for SID[%s] DN[%s], "
				  "returning INVALID_PARAMETER\n",
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (new_password != NULL && (new_password->length % 2)) {
			ldb_transaction_cancel(ldb);
			DEBUG(2, ("samdb_set_password_sid: "
				  "invalid utf16 length (%zu) "
				  "sAMAccountName for SID[%s] DN[%s], "
				  "returning WRONG_PASSWORD\n",
				  new_password->length,
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (new_password != NULL && new_password->length >= 500) {
			ldb_transaction_cancel(ldb);
			DEBUG(2, ("samdb_set_password_sid: "
				  "utf16 password too long (%zu) "
				  "sAMAccountName for SID[%s] DN[%s], "
				  "returning WRONG_PASSWORD\n",
				  new_password->length,
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_WRONG_PASSWORD;
		}

		account_name = ldb_msg_find_attr_as_string(user_msg,
							"sAMAccountName", NULL);
		if (account_name == NULL) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: missing "
				  "sAMAccountName for SID[%s] DN[%s], "
				  "returning NO_SUCH_USER\n",
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_NO_SUCH_USER;
		}

		nt_status = dsdb_trust_search_tdo_by_type(ldb,
							  SEC_CHAN_DOMAIN,
							  account_name,
							  tdo_attrs,
							  frame, &tdo_msg);
		if (!NT_STATUS_IS_OK(nt_status)) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: dsdb_trust_search_tdo "
				  "failed(%s) for sAMAccountName[%s] SID[%s] DN[%s], "
				  "returning INTERNAL_DB_CORRUPTION\n",
				  nt_errstr(nt_status), account_name,
				  dom_sid_string(frame, user_sid),
				  ldb_dn_get_linearized(user_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_INTERNAL_DB_CORRUPTION;
		}

		trust_direction = ldb_msg_find_attr_as_int(tdo_msg,
							   "trustDirection", 0);
		if (!(trust_direction & LSA_TRUST_DIRECTION_INBOUND)) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: direction[0x%08X] is "
				  "not inbound for sAMAccountName[%s] "
				  "DN[%s] TDO[%s], "
				  "returning INTERNAL_DB_CORRUPTION\n",
				  (unsigned)trust_direction,
				  account_name,
				  ldb_dn_get_linearized(user_msg->dn),
				  ldb_dn_get_linearized(tdo_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_INTERNAL_DB_CORRUPTION;
		}

		old_val = ldb_msg_find_ldb_val(tdo_msg, "trustAuthIncoming");
		if (old_val != NULL) {
			ndr_err = ndr_pull_struct_blob(old_val, frame, &old_blob,
					(ndr_pull_flags_fn_t)ndr_pull_trustAuthInOutBlob);
			if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
				ldb_transaction_cancel(ldb);
				DEBUG(1, ("samdb_set_password_sid: "
					  "failed(%s) to parse "
					  "trustAuthOutgoing sAMAccountName[%s] "
					  "DN[%s] TDO[%s], "
					  "returning INTERNAL_DB_CORRUPTION\n",
					  ndr_map_error2string(ndr_err),
					  account_name,
					  ldb_dn_get_linearized(user_msg->dn),
					  ldb_dn_get_linearized(tdo_msg->dn)));

				TALLOC_FREE(frame);
				return NT_STATUS_INTERNAL_DB_CORRUPTION;
			}
		}

		for (i = old_blob.current.count; i > 0; i--) {
			struct AuthenticationInformation *a =
				&old_blob.current.array[i - 1];

			switch (a->AuthType) {
			case TRUST_AUTH_TYPE_NONE:
				if (i == old_blob.current.count) {
					/*
					 * remove TRUST_AUTH_TYPE_NONE at the
					 * end
					 */
					old_blob.current.count--;
				}
				break;

			case TRUST_AUTH_TYPE_VERSION:
				old_version_a = a;
				old_version = a->AuthInfo.version.version;
				break;

			case TRUST_AUTH_TYPE_CLEAR:
				break;

			case TRUST_AUTH_TYPE_NT4OWF:
				break;
			}
		}

		if (new_version == NULL) {
			_new_version = 0;
			new_version = &_new_version;
		}

		if (old_version_a != NULL && *new_version != (old_version + 1)) {
			old_version_a->LastUpdateTime = now;
			old_version_a->AuthType = TRUST_AUTH_TYPE_NONE;
		}

		new_blob.count = MAX(old_blob.current.count, 2);
		new_blob.current.array = talloc_zero_array(frame,
						struct AuthenticationInformation,
						new_blob.count);
		if (new_blob.current.array == NULL) {
			ldb_transaction_cancel(ldb);
			TALLOC_FREE(frame);
			return NT_STATUS_NO_MEMORY;
		}
		new_blob.previous.array = talloc_zero_array(frame,
						struct AuthenticationInformation,
						new_blob.count);
		if (new_blob.current.array == NULL) {
			ldb_transaction_cancel(ldb);
			TALLOC_FREE(frame);
			return NT_STATUS_NO_MEMORY;
		}

		for (i = 0; i < old_blob.current.count; i++) {
			struct AuthenticationInformation *o =
				&old_blob.current.array[i];
			struct AuthenticationInformation *p =
				&new_blob.previous.array[i];

			*p = *o;
			new_blob.previous.count++;
		}
		for (; i < new_blob.count; i++) {
			struct AuthenticationInformation *pi =
				&new_blob.previous.array[i];

			if (i == 0) {
				/*
				 * new_blob.previous is still empty so
				 * we'll do new_blob.previous = new_blob.current
				 * below.
				 */
				break;
			}

			pi->LastUpdateTime = now;
			pi->AuthType = TRUST_AUTH_TYPE_NONE;
			new_blob.previous.count++;
		}

		for (i = 0; i < new_blob.count; i++) {
			struct AuthenticationInformation *ci =
				&new_blob.current.array[i];

			ci->LastUpdateTime = now;
			switch (i) {
			case 0:
				if (ntNewHash != NULL) {
					ci->AuthType = TRUST_AUTH_TYPE_NT4OWF;
					ci->AuthInfo.nt4owf.password = *ntNewHash;
					break;
				}

				ci->AuthType = TRUST_AUTH_TYPE_CLEAR;
				ci->AuthInfo.clear.size = new_password->length;
				ci->AuthInfo.clear.password = new_password->data;
				break;
			case 1:
				ci->AuthType = TRUST_AUTH_TYPE_VERSION;
				ci->AuthInfo.version.version = *new_version;
				break;
			default:
				ci->AuthType = TRUST_AUTH_TYPE_NONE;
				break;
			}

			new_blob.current.count++;
		}

		if (new_blob.previous.count == 0) {
			TALLOC_FREE(new_blob.previous.array);
			new_blob.previous = new_blob.current;
		}

		ndr_err = ndr_push_struct_blob(&new_val, frame, &new_blob,
				(ndr_push_flags_fn_t)ndr_push_trustAuthInOutBlob);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: "
				  "failed(%s) to generate "
				  "trustAuthOutgoing sAMAccountName[%s] "
				  "DN[%s] TDO[%s], "
				  "returning UNSUCCESSFUL\n",
				  ndr_map_error2string(ndr_err),
				  account_name,
				  ldb_dn_get_linearized(user_msg->dn),
				  ldb_dn_get_linearized(tdo_msg->dn)));
			TALLOC_FREE(frame);
			return NT_STATUS_UNSUCCESSFUL;
		}

		tdo_msg->num_elements = 0;
		TALLOC_FREE(tdo_msg->elements);

		ret = ldb_msg_add_empty(tdo_msg, "trustAuthIncoming",
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			ldb_transaction_cancel(ldb);
			TALLOC_FREE(frame);
			return NT_STATUS_NO_MEMORY;
		}
		ret = ldb_msg_add_value(tdo_msg, "trustAuthIncoming",
					&new_val, NULL);
		if (ret != LDB_SUCCESS) {
			ldb_transaction_cancel(ldb);
			TALLOC_FREE(frame);
			return NT_STATUS_NO_MEMORY;
		}

		ret = ldb_modify(ldb, tdo_msg);
		if (ret != LDB_SUCCESS) {
			nt_status = dsdb_ldb_err_to_ntstatus(ret);
			ldb_transaction_cancel(ldb);
			DEBUG(1, ("samdb_set_password_sid: "
				  "failed to replace "
				  "trustAuthOutgoing sAMAccountName[%s] "
				  "DN[%s] TDO[%s], "
				  "%s - %s\n",
				  account_name,
				  ldb_dn_get_linearized(user_msg->dn),
				  ldb_dn_get_linearized(tdo_msg->dn),
				  nt_errstr(nt_status), ldb_errstring(ldb)));
			TALLOC_FREE(frame);
			return nt_status;
		}
	}

	nt_status = samdb_set_password_internal(ldb, mem_ctx,
						user_msg->dn, NULL,
						new_password,
						ntNewHash,
						old_password_checked,
						reject_reason, _dominfo,
						true); /* permit trusts */
	if (!NT_STATUS_IS_OK(nt_status)) {
		ldb_transaction_cancel(ldb);
		TALLOC_FREE(frame);
		return nt_status;
	}

	ret = ldb_transaction_commit(ldb);
	if (ret != LDB_SUCCESS) {
		DEBUG(0,("Failed to commit transaction to change password on %s: %s\n",
			 ldb_dn_get_linearized(user_msg->dn),
			 ldb_errstring(ldb)));
		TALLOC_FREE(frame);
		return NT_STATUS_TRANSACTION_ABORTED;
	}

	TALLOC_FREE(frame);
	return NT_STATUS_OK;
}