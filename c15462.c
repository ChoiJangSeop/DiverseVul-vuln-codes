static bool pdb_samba_dsdb_set_trusteddom_pw(struct pdb_methods *m,
				      const char* domain, const char* pwd,
				      const struct dom_sid *sid)
{
	struct pdb_samba_dsdb_state *state = talloc_get_type_abort(
		m->private_data, struct pdb_samba_dsdb_state);
	TALLOC_CTX *tmp_ctx = talloc_stackframe();
	const char * const attrs[] = {
		"trustAuthOutgoing",
		"trustDirection",
		"trustType",
		NULL
	};
	struct ldb_message *msg = NULL;
	int trust_direction_flags;
	int trust_type;
	uint32_t i; /* The same type as old_blob.current.count */
	const struct ldb_val *old_val = NULL;
	struct trustAuthInOutBlob old_blob = {};
	uint32_t old_version = 0;
	uint32_t new_version = 0;
	DATA_BLOB new_utf16 = {};
	struct trustAuthInOutBlob new_blob = {};
	struct ldb_val new_val = {};
	struct timeval tv = timeval_current();
	NTTIME now = timeval_to_nttime(&tv);
	enum ndr_err_code ndr_err;
	NTSTATUS status;
	bool ok;
	int ret;

	ret = ldb_transaction_start(state->ldb);
	if (ret != LDB_SUCCESS) {
		DEBUG(2, ("Failed to start transaction.\n"));
		TALLOC_FREE(tmp_ctx);
		return false;
	}

	ok = samdb_is_pdc(state->ldb);
	if (!ok) {
		DEBUG(2, ("Password changes for domain %s are only allowed on a PDC.\n",
			  domain));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	status = dsdb_trust_search_tdo(state->ldb, domain, NULL,
				       attrs, tmp_ctx, &msg);
	if (!NT_STATUS_IS_OK(status)) {
		/*
		 * This can be called to work out of a domain is
		 * trusted, rather than just to get the password
		 */
		DEBUG(2, ("Failed to get trusted domain password for %s - %s.  "
			  "It may not be a trusted domain.\n", domain,
			  nt_errstr(status)));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	trust_direction_flags = ldb_msg_find_attr_as_int(msg, "trustDirection", 0);
	if (!(trust_direction_flags & LSA_TRUST_DIRECTION_OUTBOUND)) {
		DBG_WARNING("Trusted domain %s is not an outbound trust, can't set a password.\n",
			    domain);
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	trust_type = ldb_msg_find_attr_as_int(msg, "trustType", 0);
	switch (trust_type) {
	case LSA_TRUST_TYPE_DOWNLEVEL:
	case LSA_TRUST_TYPE_UPLEVEL:
		break;
	default:
		DEBUG(0, ("Trusted domain %s is of type 0x%X - "
			  "password changes are not supported\n",
			  domain, (unsigned)trust_type));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	old_val = ldb_msg_find_ldb_val(msg, "trustAuthOutgoing");
	if (old_val != NULL) {
		ndr_err = ndr_pull_struct_blob(old_val, tmp_ctx, &old_blob,
				(ndr_pull_flags_fn_t)ndr_pull_trustAuthInOutBlob);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			DEBUG(0, ("Failed to get trusted domain password for %s, "
				  "attribute trustAuthOutgoing could not be parsed %s.\n",
				  domain,
				  ndr_map_error2string(ndr_err)));
			TALLOC_FREE(tmp_ctx);
			ldb_transaction_cancel(state->ldb);
			return false;
		}
	}

	for (i=0; i < old_blob.current.count; i++) {
		struct AuthenticationInformation *a =
			&old_blob.current.array[i];

		switch (a->AuthType) {
		case TRUST_AUTH_TYPE_NONE:
			break;

		case TRUST_AUTH_TYPE_VERSION:
			old_version = a->AuthInfo.version.version;
			break;

		case TRUST_AUTH_TYPE_CLEAR:
			break;

		case TRUST_AUTH_TYPE_NT4OWF:
			break;
		}
	}

	new_version = old_version + 1;
	ok = convert_string_talloc(tmp_ctx,
				   CH_UNIX, CH_UTF16,
				   pwd, strlen(pwd),
			           (void *)&new_utf16.data,
			           &new_utf16.length);
	if (!ok) {
		DEBUG(0, ("Failed to generate new_utf16 password for  domain %s\n",
			  domain));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	if (new_utf16.length < 28) {
		DEBUG(0, ("new_utf16[%zu] version[%u] for domain %s to short.\n",
			  new_utf16.length,
			  (unsigned)new_version,
			  domain));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}
	if (new_utf16.length > 498) {
		DEBUG(0, ("new_utf16[%zu] version[%u] for domain %s to long.\n",
			  new_utf16.length,
			  (unsigned)new_version,
			  domain));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	new_blob.count = MAX(old_blob.current.count, 2);
	new_blob.current.array = talloc_zero_array(tmp_ctx,
					struct AuthenticationInformation,
					new_blob.count);
	if (new_blob.current.array == NULL) {
		DEBUG(0, ("talloc_zero_array(%u) failed\n",
			  (unsigned)new_blob.count));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}
	new_blob.previous.array = talloc_zero_array(tmp_ctx,
					struct AuthenticationInformation,
					new_blob.count);
	if (new_blob.current.array == NULL) {
		DEBUG(0, ("talloc_zero_array(%u) failed\n",
			  (unsigned)new_blob.count));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
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
			ci->AuthType = TRUST_AUTH_TYPE_CLEAR;
			ci->AuthInfo.clear.size = new_utf16.length;
			ci->AuthInfo.clear.password = new_utf16.data;
			break;
		case 1:
			ci->AuthType = TRUST_AUTH_TYPE_VERSION;
			ci->AuthInfo.version.version = new_version;
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

	ndr_err = ndr_push_struct_blob(&new_val, tmp_ctx, &new_blob,
			(ndr_push_flags_fn_t)ndr_push_trustAuthInOutBlob);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		DEBUG(0, ("Failed to generate trustAuthOutgoing for "
			  "trusted domain password for %s: %s.\n",
			  domain, ndr_map_error2string(ndr_err)));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	msg->num_elements = 0;
	ret = ldb_msg_add_empty(msg, "trustAuthOutgoing",
				LDB_FLAG_MOD_REPLACE, NULL);
	if (ret != LDB_SUCCESS) {
		DEBUG(0, ("ldb_msg_add_empty() failed\n"));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}
	ret = ldb_msg_add_value(msg, "trustAuthOutgoing",
				&new_val, NULL);
	if (ret != LDB_SUCCESS) {
		DEBUG(0, ("ldb_msg_add_value() failed\n"));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	ret = ldb_modify(state->ldb, msg);
	if (ret != LDB_SUCCESS) {
		DEBUG(0, ("Failed to replace trustAuthOutgoing for "
			  "trusted domain password for %s: %s - %s\n",
			  domain, ldb_strerror(ret), ldb_errstring(state->ldb)));
		TALLOC_FREE(tmp_ctx);
		ldb_transaction_cancel(state->ldb);
		return false;
	}

	ret = ldb_transaction_commit(state->ldb);
	if (ret != LDB_SUCCESS) {
		DEBUG(0, ("Failed to commit trustAuthOutgoing for "
			  "trusted domain password for %s: %s - %s\n",
			  domain, ldb_strerror(ret), ldb_errstring(state->ldb)));
		TALLOC_FREE(tmp_ctx);
		return false;
	}

	DEBUG(1, ("Added new_version[%u] to trustAuthOutgoing for "
		  "trusted domain password for %s.\n",
		  (unsigned)new_version, domain));
	TALLOC_FREE(tmp_ctx);
	return true;
}