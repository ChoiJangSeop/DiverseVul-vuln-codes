static NTSTATUS dcesrv_lsa_lsaRSetForestTrustInformation(struct dcesrv_call_state *dce_call,
							 TALLOC_CTX *mem_ctx,
							 struct lsa_lsaRSetForestTrustInformation *r)
{
	struct dcesrv_handle *h;
	struct lsa_policy_state *p_state;
	const char * const trust_attrs[] = {
		"securityIdentifier",
		"flatName",
		"trustPartner",
		"trustAttributes",
		"trustDirection",
		"trustType",
		"msDS-TrustForestTrustInfo",
		NULL
	};
	struct ldb_message *trust_tdo_msg = NULL;
	struct lsa_TrustDomainInfoInfoEx *trust_tdo = NULL;
	struct lsa_ForestTrustInformation *step1_lfti = NULL;
	struct lsa_ForestTrustInformation *step2_lfti = NULL;
	struct ForestTrustInfo *trust_fti = NULL;
	struct ldb_result *trusts_res = NULL;
	unsigned int i;
	struct lsa_TrustDomainInfoInfoEx *xref_tdo = NULL;
	struct lsa_ForestTrustInformation *xref_lfti = NULL;
	struct lsa_ForestTrustCollisionInfo *c_info = NULL;
	DATA_BLOB ft_blob = {};
	struct ldb_message *msg = NULL;
	struct server_id *server_ids = NULL;
	uint32_t num_server_ids = 0;
	NTSTATUS status;
	enum ndr_err_code ndr_err;
	int ret;
	bool in_transaction = false;
	struct imessaging_context *imsg_ctx =
		dcesrv_imessaging_context(dce_call->conn);

	DCESRV_PULL_HANDLE(h, r->in.handle, LSA_HANDLE_POLICY);

	p_state = h->data;

	if (strcmp(p_state->domain_dns, p_state->forest_dns)) {
		return NT_STATUS_INVALID_DOMAIN_STATE;
	}

	if (r->in.check_only == 0) {
		ret = ldb_transaction_start(p_state->sam_ldb);
		if (ret != LDB_SUCCESS) {
			return NT_STATUS_INTERNAL_DB_CORRUPTION;
		}
		in_transaction = true;
	}

	/*
	 * abort if we are not a PDC
	 *
	 * In future we should use a function like IsEffectiveRoleOwner()
	 */
	if (!samdb_is_pdc(p_state->sam_ldb)) {
		status = NT_STATUS_INVALID_DOMAIN_ROLE;
		goto done;
	}

	if (r->in.trusted_domain_name->string == NULL) {
		status = NT_STATUS_NO_SUCH_DOMAIN;
		goto done;
	}

	status = dsdb_trust_search_tdo(p_state->sam_ldb,
				       r->in.trusted_domain_name->string,
				       r->in.trusted_domain_name->string,
				       trust_attrs, mem_ctx, &trust_tdo_msg);
	if (NT_STATUS_EQUAL(status, NT_STATUS_OBJECT_NAME_NOT_FOUND)) {
		status = NT_STATUS_NO_SUCH_DOMAIN;
		goto done;
	}
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	status = dsdb_trust_parse_tdo_info(mem_ctx, trust_tdo_msg, &trust_tdo);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	if (!(trust_tdo->trust_attributes & LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE)) {
		status = NT_STATUS_INVALID_PARAMETER;
		goto done;
	}

	if (r->in.highest_record_type >= LSA_FOREST_TRUST_RECORD_TYPE_LAST) {
		status = NT_STATUS_INVALID_PARAMETER;
		goto done;
	}

	/*
	 * verify and normalize the given forest trust info.
	 *
	 * Step1: doesn't reorder yet, so step1_lfti might contain
	 * NULL entries. This means dsdb_trust_verify_forest_info()
	 * can generate collision entries with the callers index.
	 */
	status = dsdb_trust_normalize_forest_info_step1(mem_ctx,
							r->in.forest_trust_info,
							&step1_lfti);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	c_info = talloc_zero(r->out.collision_info,
			     struct lsa_ForestTrustCollisionInfo);
	if (c_info == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	/*
	 * First check our own forest, then other domains/forests
	 */

	status = dsdb_trust_xref_tdo_info(mem_ctx, p_state->sam_ldb,
					  &xref_tdo);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}
	status = dsdb_trust_xref_forest_info(mem_ctx, p_state->sam_ldb,
					     &xref_lfti);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	/*
	 * The documentation proposed to generate
	 * LSA_FOREST_TRUST_COLLISION_XREF collisions.
	 * But Windows always uses LSA_FOREST_TRUST_COLLISION_TDO.
	 */
	status = dsdb_trust_verify_forest_info(xref_tdo, xref_lfti,
					       LSA_FOREST_TRUST_COLLISION_TDO,
					       c_info, step1_lfti);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	/* fetch all other trusted domain objects */
	status = dsdb_trust_search_tdos(p_state->sam_ldb,
					trust_tdo->domain_name.string,
					trust_attrs,
					mem_ctx, &trusts_res);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	/*
	 * now check against the other domains.
	 * and generate LSA_FOREST_TRUST_COLLISION_TDO collisions.
	 */
	for (i = 0; i < trusts_res->count; i++) {
		struct lsa_TrustDomainInfoInfoEx *tdo = NULL;
		struct ForestTrustInfo *fti = NULL;
		struct lsa_ForestTrustInformation *lfti = NULL;

		status = dsdb_trust_parse_tdo_info(mem_ctx,
						   trusts_res->msgs[i],
						   &tdo);
		if (!NT_STATUS_IS_OK(status)) {
			goto done;
		}

		status = dsdb_trust_parse_forest_info(tdo,
						      trusts_res->msgs[i],
						      &fti);
		if (NT_STATUS_EQUAL(status, NT_STATUS_NOT_FOUND)) {
			continue;
		}
		if (!NT_STATUS_IS_OK(status)) {
			goto done;
		}

		status = dsdb_trust_forest_info_to_lsa(tdo, fti, &lfti);
		if (!NT_STATUS_IS_OK(status)) {
			goto done;
		}

		status = dsdb_trust_verify_forest_info(tdo, lfti,
						LSA_FOREST_TRUST_COLLISION_TDO,
						c_info, step1_lfti);
		if (!NT_STATUS_IS_OK(status)) {
			goto done;
		}

		TALLOC_FREE(tdo);
	}

	if (r->in.check_only != 0) {
		status = NT_STATUS_OK;
		goto done;
	}

	/*
	 * not just a check, write info back
	 */

	/*
	 * normalize the given forest trust info.
	 *
	 * Step2: adds TOP_LEVEL_NAME[_EX] in reverse order,
	 * followed by DOMAIN_INFO in reverse order. It also removes
	 * possible NULL entries from Step1.
	 */
	status = dsdb_trust_normalize_forest_info_step2(mem_ctx, step1_lfti,
							&step2_lfti);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	status = dsdb_trust_forest_info_from_lsa(mem_ctx, step2_lfti,
						 &trust_fti);
	if (!NT_STATUS_IS_OK(status)) {
		goto done;
	}

	ndr_err = ndr_push_struct_blob(&ft_blob, mem_ctx, trust_fti,
				       (ndr_push_flags_fn_t)ndr_push_ForestTrustInfo);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		status = NT_STATUS_INVALID_PARAMETER;
		goto done;
	}

	msg = ldb_msg_new(mem_ctx);
	if (msg == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	msg->dn = ldb_dn_copy(mem_ctx, trust_tdo_msg->dn);
	if (!msg->dn) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	ret = ldb_msg_add_empty(msg, "msDS-TrustForestTrustInfo",
				LDB_FLAG_MOD_REPLACE, NULL);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}
	ret = ldb_msg_add_value(msg, "msDS-TrustForestTrustInfo",
				&ft_blob, NULL);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	ret = ldb_modify(p_state->sam_ldb, msg);
	if (ret != LDB_SUCCESS) {
		status = dsdb_ldb_err_to_ntstatus(ret);

		DEBUG(0, ("Failed to store Forest Trust Info: %s\n",
			  ldb_errstring(p_state->sam_ldb)));

		goto done;
	}

	/* ok, all fine, commit transaction and return */
	in_transaction = false;
	ret = ldb_transaction_commit(p_state->sam_ldb);
	if (ret != LDB_SUCCESS) {
		status = NT_STATUS_INTERNAL_DB_CORRUPTION;
		goto done;
	}

	/*
	 * Notify winbindd that we have a acquired forest trust info
	 */
	status = irpc_servers_byname(imsg_ctx,
				     mem_ctx,
				     "winbind_server",
				     &num_server_ids,
				     &server_ids);
	if (!NT_STATUS_IS_OK(status)) {
		DBG_ERR("irpc_servers_byname failed\n");
		goto done;
	}

	imessaging_send(imsg_ctx,
			server_ids[0],
			MSG_WINBIND_RELOAD_TRUSTED_DOMAINS,
			NULL);

	status = NT_STATUS_OK;

done:
	if (NT_STATUS_IS_OK(status) && c_info->count != 0) {
		*r->out.collision_info = c_info;
	}

	if (in_transaction) {
		ldb_transaction_cancel(p_state->sam_ldb);
	}

	return status;
}