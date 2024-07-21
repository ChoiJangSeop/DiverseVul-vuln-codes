static NTSTATUS setInfoTrustedDomain_base(struct dcesrv_call_state *dce_call,
					  struct lsa_policy_state *p_state,
					  TALLOC_CTX *mem_ctx,
					  struct ldb_message *dom_msg,
					  enum lsa_TrustDomInfoEnum level,
					  union lsa_TrustedDomainInfo *info)
{
	uint32_t *posix_offset = NULL;
	struct lsa_TrustDomainInfoInfoEx *info_ex = NULL;
	struct lsa_TrustDomainInfoAuthInfo *auth_info = NULL;
	struct lsa_TrustDomainInfoAuthInfoInternal *auth_info_int = NULL;
	uint32_t *enc_types = NULL;
	DATA_BLOB trustAuthIncoming, trustAuthOutgoing, auth_blob;
	struct trustDomainPasswords auth_struct;
	struct trustAuthInOutBlob *current_passwords = NULL;
	NTSTATUS nt_status;
	struct ldb_message **msgs;
	struct ldb_message *msg;
	bool add_outgoing = false;
	bool add_incoming = false;
	bool del_outgoing = false;
	bool del_incoming = false;
	bool del_forest_info = false;
	bool in_transaction = false;
	int ret;
	bool am_rodc;

	switch (level) {
	case LSA_TRUSTED_DOMAIN_INFO_POSIX_OFFSET:
		posix_offset = &info->posix_offset.posix_offset;
		break;
	case LSA_TRUSTED_DOMAIN_INFO_INFO_EX:
		info_ex = &info->info_ex;
		break;
	case LSA_TRUSTED_DOMAIN_INFO_AUTH_INFO:
		auth_info = &info->auth_info;
		break;
	case LSA_TRUSTED_DOMAIN_INFO_FULL_INFO:
		posix_offset = &info->full_info.posix_offset.posix_offset;
		info_ex = &info->full_info.info_ex;
		auth_info = &info->full_info.auth_info;
		break;
	case LSA_TRUSTED_DOMAIN_INFO_AUTH_INFO_INTERNAL:
		auth_info_int = &info->auth_info_internal;
		break;
	case LSA_TRUSTED_DOMAIN_INFO_FULL_INFO_INTERNAL:
		posix_offset = &info->full_info_internal.posix_offset.posix_offset;
		info_ex = &info->full_info_internal.info_ex;
		auth_info_int = &info->full_info_internal.auth_info;
		break;
	case LSA_TRUSTED_DOMAIN_SUPPORTED_ENCRYPTION_TYPES:
		enc_types = &info->enc_types.enc_types;
		break;
	default:
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (auth_info) {
		nt_status = auth_info_2_auth_blob(mem_ctx, auth_info,
						  &trustAuthIncoming,
						  &trustAuthOutgoing);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
		if (trustAuthIncoming.data) {
			/* This does the decode of some of this twice, but it is easier that way */
			nt_status = auth_info_2_trustauth_inout(mem_ctx,
								auth_info->incoming_count,
								auth_info->incoming_current_auth_info,
								NULL,
								&current_passwords);
			if (!NT_STATUS_IS_OK(nt_status)) {
				return nt_status;
			}
		}
	}

	/* decode auth_info_int if set */
	if (auth_info_int) {

		/* now decrypt blob */
		auth_blob = data_blob_const(auth_info_int->auth_blob.data,
					    auth_info_int->auth_blob.size);

		nt_status = get_trustdom_auth_blob(dce_call, mem_ctx,
						   &auth_blob, &auth_struct);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
	}

	if (info_ex) {
		/* verify data matches */
		if (info_ex->trust_attributes &
		    LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE) {
			/* TODO: check what behavior level we have */
		       if (strcasecmp_m(p_state->domain_dns,
					p_state->forest_dns) != 0) {
				return NT_STATUS_INVALID_DOMAIN_STATE;
			}
		}

		ret = samdb_rodc(p_state->sam_ldb, &am_rodc);
		if (ret == LDB_SUCCESS && am_rodc) {
			return NT_STATUS_NO_SUCH_DOMAIN;
		}

		/* verify only one object matches the dns/netbios/sid
		 * triplet and that this is the one we already have */
		nt_status = get_tdo(p_state->sam_ldb, mem_ctx,
				    p_state->system_dn,
				    info_ex->domain_name.string,
				    info_ex->netbios_name.string,
				    info_ex->sid, &msgs);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
		if (ldb_dn_compare(dom_msg->dn, msgs[0]->dn) != 0) {
			return NT_STATUS_OBJECT_NAME_COLLISION;
		}
		talloc_free(msgs);
	}

	/* TODO: should we fetch previous values from the existing entry
	 * and append them ? */
	if (auth_info_int && auth_struct.incoming.count) {
		nt_status = get_trustauth_inout_blob(dce_call, mem_ctx,
						     &auth_struct.incoming,
						     &trustAuthIncoming);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}

		current_passwords = &auth_struct.incoming;

	} else {
		trustAuthIncoming = data_blob(NULL, 0);
	}

	if (auth_info_int && auth_struct.outgoing.count) {
		nt_status = get_trustauth_inout_blob(dce_call, mem_ctx,
						     &auth_struct.outgoing,
						     &trustAuthOutgoing);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
	} else {
		trustAuthOutgoing = data_blob(NULL, 0);
	}

	msg = ldb_msg_new(mem_ctx);
	if (msg == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	msg->dn = dom_msg->dn;

	if (posix_offset) {
		nt_status = update_uint32_t_value(mem_ctx, p_state->sam_ldb,
						  dom_msg, msg,
						  "trustPosixOffset",
						  *posix_offset, NULL);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
	}

	if (info_ex) {
		uint32_t origattrs;
		uint32_t changed_attrs;
		uint32_t origdir;
		int origtype;

		nt_status = update_uint32_t_value(mem_ctx, p_state->sam_ldb,
						  dom_msg, msg,
						  "trustDirection",
						  info_ex->trust_direction,
						  &origdir);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}

		if (info_ex->trust_direction & LSA_TRUST_DIRECTION_INBOUND) {
			if (auth_info != NULL && trustAuthIncoming.length > 0) {
				add_incoming = true;
			}
		}
		if (info_ex->trust_direction & LSA_TRUST_DIRECTION_OUTBOUND) {
			if (auth_info != NULL && trustAuthOutgoing.length > 0) {
				add_outgoing = true;
			}
		}

		if ((origdir & LSA_TRUST_DIRECTION_INBOUND) &&
		    !(info_ex->trust_direction & LSA_TRUST_DIRECTION_INBOUND)) {
			del_incoming = true;
		}
		if ((origdir & LSA_TRUST_DIRECTION_OUTBOUND) &&
		    !(info_ex->trust_direction & LSA_TRUST_DIRECTION_OUTBOUND)) {
			del_outgoing = true;
		}

		origtype = ldb_msg_find_attr_as_int(dom_msg, "trustType", -1);
		if (origtype == -1 || origtype != info_ex->trust_type) {
			DEBUG(1, ("Attempted to change trust type! "
				  "Operation not handled\n"));
			return NT_STATUS_INVALID_PARAMETER;
		}

		nt_status = update_uint32_t_value(mem_ctx, p_state->sam_ldb,
						  dom_msg, msg,
						  "trustAttributes",
						  info_ex->trust_attributes,
						  &origattrs);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
		/* TODO: check forestFunctionality from ldb opaque */
		/* TODO: check what is set makes sense */

		changed_attrs = origattrs ^ info_ex->trust_attributes;
		if (changed_attrs & ~LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE) {
			/*
			 * For now we only allow
			 * LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE to be changed.
			 *
			 * TODO: we may need to support more attribute changes
			 */
			DEBUG(1, ("Attempted to change trust attributes "
				  "(0x%08x != 0x%08x)! "
				  "Operation not handled yet...\n",
				  (unsigned)origattrs,
				  (unsigned)info_ex->trust_attributes));
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (!(info_ex->trust_attributes &
		      LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE))
		{
			struct ldb_message_element *orig_forest_el = NULL;

			orig_forest_el = ldb_msg_find_element(dom_msg,
						"msDS-TrustForestTrustInfo");
			if (orig_forest_el != NULL) {
				del_forest_info = true;
			}
		}
	}

	if (enc_types) {
		nt_status = update_uint32_t_value(mem_ctx, p_state->sam_ldb,
						  dom_msg, msg,
						  "msDS-SupportedEncryptionTypes",
						  *enc_types, NULL);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}
	}

	if (add_incoming || del_incoming) {
		ret = ldb_msg_add_empty(msg, "trustAuthIncoming",
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_NO_MEMORY;
		}
		if (add_incoming) {
			ret = ldb_msg_add_value(msg, "trustAuthIncoming",
						&trustAuthIncoming, NULL);
			if (ret != LDB_SUCCESS) {
				return NT_STATUS_NO_MEMORY;
			}
		}
	}
	if (add_outgoing || del_outgoing) {
		ret = ldb_msg_add_empty(msg, "trustAuthOutgoing",
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_NO_MEMORY;
		}
		if (add_outgoing) {
			ret = ldb_msg_add_value(msg, "trustAuthOutgoing",
						&trustAuthOutgoing, NULL);
			if (ret != LDB_SUCCESS) {
				return NT_STATUS_NO_MEMORY;
			}
		}
	}
	if (del_forest_info) {
		ret = ldb_msg_add_empty(msg, "msDS-TrustForestTrustInfo",
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_NO_MEMORY;
		}
	}

	/* start transaction */
	ret = ldb_transaction_start(p_state->sam_ldb);
	if (ret != LDB_SUCCESS) {
		return NT_STATUS_INTERNAL_DB_CORRUPTION;
	}
	in_transaction = true;

	if (msg->num_elements) {
		ret = ldb_modify(p_state->sam_ldb, msg);
		if (ret != LDB_SUCCESS) {
			DEBUG(1,("Failed to modify trusted domain record %s: %s\n",
				 ldb_dn_get_linearized(msg->dn),
				 ldb_errstring(p_state->sam_ldb)));
			nt_status = dsdb_ldb_err_to_ntstatus(ret);
			goto done;
		}
	}

	if (add_incoming || del_incoming) {
		const char *netbios_name;

		netbios_name = ldb_msg_find_attr_as_string(dom_msg,
							   "flatname", NULL);
		if (!netbios_name) {
			nt_status = NT_STATUS_INVALID_DOMAIN_STATE;
			goto done;
		}

		/* We use trustAuthIncoming.data to incidate that auth_struct.incoming is valid */
		nt_status = update_trust_user(mem_ctx,
					      p_state->sam_ldb,
					      p_state->domain_dn,
					      del_incoming,
					      netbios_name,
					      current_passwords);
		if (!NT_STATUS_IS_OK(nt_status)) {
			goto done;
		}
	}

	/* ok, all fine, commit transaction and return */
	ret = ldb_transaction_commit(p_state->sam_ldb);
	if (ret != LDB_SUCCESS) {
		return NT_STATUS_INTERNAL_DB_CORRUPTION;
	}
	in_transaction = false;

	nt_status = NT_STATUS_OK;

done:
	if (in_transaction) {
		ldb_transaction_cancel(p_state->sam_ldb);
	}
	return nt_status;
}