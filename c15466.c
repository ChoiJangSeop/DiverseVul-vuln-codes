static int replmd_rename_callback(struct ldb_request *req, struct ldb_reply *ares)
{
	struct ldb_context *ldb;
	struct ldb_request *down_req;
	struct ldb_message *msg;
	const struct dsdb_attribute *rdn_attr;
	const char *rdn_name;
	const struct ldb_val *rdn_val;
	const char *attrs[5] = { NULL, };
	time_t t = time(NULL);
	int ret;
	bool is_urgent = false, rodc = false;
	bool is_schema_nc;
	struct replmd_replicated_request *ac =
		talloc_get_type(req->context, struct replmd_replicated_request);
	struct replmd_private *replmd_private =
		talloc_get_type(ldb_module_get_private(ac->module),
				struct replmd_private);

	ldb = ldb_module_get_ctx(ac->module);

	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	if (ares->type != LDB_REPLY_DONE) {
		ldb_set_errstring(ldb,
			"invalid reply type in repl_meta_data rename callback");
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
					LDB_ERR_OPERATIONS_ERROR);
	}

	/* TODO:
	 * - replace the old object with the newly constructed one
	 */

	msg = ldb_msg_new(ac);
	if (msg == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	msg->dn = ac->req->op.rename.newdn;

	is_schema_nc = ldb_dn_compare_base(replmd_private->schema_dn, msg->dn) == 0;

	rdn_name = ldb_dn_get_rdn_name(msg->dn);
	if (rdn_name == NULL) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_operr(ldb));
	}

	/* normalize the rdn attribute name */
	rdn_attr = dsdb_attribute_by_lDAPDisplayName(ac->schema, rdn_name);
	if (rdn_attr == NULL) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_operr(ldb));
	}
	rdn_name = rdn_attr->lDAPDisplayName;

	rdn_val = ldb_dn_get_rdn_val(msg->dn);
	if (rdn_val == NULL) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_operr(ldb));
	}

	if (ldb_msg_add_empty(msg, rdn_name, LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_oom(ldb));
	}
	if (ldb_msg_add_value(msg, rdn_name, rdn_val, NULL) != 0) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_oom(ldb));
	}
	if (ldb_msg_add_empty(msg, "name", LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_oom(ldb));
	}
	if (ldb_msg_add_value(msg, "name", rdn_val, NULL) != 0) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_oom(ldb));
	}

	/*
	 * here we let replmd_update_rpmd() only search for
	 * the existing "replPropertyMetaData" and rdn_name attributes.
	 *
	 * We do not want the existing "name" attribute as
	 * the "name" attribute needs to get the version
	 * updated on rename even if the rdn value hasn't changed.
	 *
	 * This is the diff of the meta data, for a moved user
	 * on a w2k8r2 server:
	 *
	 * # record 1
	 * -dn: CN=sdf df,CN=Users,DC=bla,DC=base
	 * +dn: CN=sdf df,OU=TestOU,DC=bla,DC=base
	 *  replPropertyMetaData:     NDR: struct replPropertyMetaDataBlob
	 *         version                  : 0x00000001 (1)
	 *         reserved                 : 0x00000000 (0)
	 * @@ -66,11 +66,11 @@ replPropertyMetaData:     NDR: struct re
	 *                      local_usn                : 0x00000000000037a5 (14245)
	 *                 array: struct replPropertyMetaData1
	 *                      attid                    : DRSUAPI_ATTID_name (0x90001)
	 * -                    version                  : 0x00000001 (1)
	 * -                    originating_change_time  : Wed Feb  9 17:20:49 2011 CET
	 * +                    version                  : 0x00000002 (2)
	 * +                    originating_change_time  : Wed Apr  6 15:21:01 2011 CEST
	 *                      originating_invocation_id: 0d36ca05-5507-4e62-aca3-354bab0d39e1
	 * -                    originating_usn          : 0x00000000000037a5 (14245)
	 * -                    local_usn                : 0x00000000000037a5 (14245)
	 * +                    originating_usn          : 0x0000000000003834 (14388)
	 * +                    local_usn                : 0x0000000000003834 (14388)
	 *                 array: struct replPropertyMetaData1
	 *                      attid                    : DRSUAPI_ATTID_userAccountControl (0x90008)
	 *                      version                  : 0x00000004 (4)
	 */
	attrs[0] = "replPropertyMetaData";
	attrs[1] = "objectClass";
	attrs[2] = "instanceType";
	attrs[3] = rdn_name;
	attrs[4] = NULL;

	ret = replmd_update_rpmd(ac->module, ac->schema, req, attrs,
				 msg, &ac->seq_num, t,
				 is_schema_nc, &is_urgent, &rodc);
	if (rodc && (ret == LDB_ERR_REFERRAL)) {
		ret = send_rodc_referral(req, ldb, ac->req->op.rename.olddn);
		talloc_free(ares);
		return ldb_module_done(req, NULL, NULL, ret);
	}

	if (ret != LDB_SUCCESS) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL, ret);
	}

	if (ac->seq_num == 0) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
				       ldb_error(ldb, ret,
					"internal error seq_num == 0"));
	}
	ac->is_urgent = is_urgent;

	ret = ldb_build_mod_req(&down_req, ldb, ac,
				msg,
				req->controls,
				ac, replmd_op_callback,
				req);
	LDB_REQ_SET_LOCATION(down_req);
	if (ret != LDB_SUCCESS) {
		talloc_free(ac);
		return ret;
	}

	/* current partition control is needed by "replmd_op_callback" */
	if (ldb_request_get_control(req, DSDB_CONTROL_CURRENT_PARTITION_OID) == NULL) {
		ret = ldb_request_add_control(down_req,
					      DSDB_CONTROL_CURRENT_PARTITION_OID,
					      false, NULL);
		if (ret != LDB_SUCCESS) {
			talloc_free(ac);
			return ret;
		}
	}

	talloc_steal(down_req, msg);

	ret = add_time_element(msg, "whenChanged", t);
	if (ret != LDB_SUCCESS) {
		talloc_free(ac);
		ldb_operr(ldb);
		return ret;
	}

	ret = add_uint64_element(ldb, msg, "uSNChanged", ac->seq_num);
	if (ret != LDB_SUCCESS) {
		talloc_free(ac);
		ldb_operr(ldb);
		return ret;
	}

	/* go on with the call chain - do the modify after the rename */
	return ldb_next_request(ac->module, down_req);
}