static int replmd_name_modify(struct replmd_replicated_request *ar,
			      struct ldb_request *req, struct ldb_dn *dn)
{
	struct ldb_message *msg;
	const char *rdn_name;
	const struct ldb_val *rdn_val;
	const struct dsdb_attribute *rdn_attr;
	int ret;

	msg = ldb_msg_new(req);
	if (msg == NULL) {
		goto failed;
	}
	msg->dn = dn;

	rdn_name = ldb_dn_get_rdn_name(dn);
	if (rdn_name == NULL) {
		goto failed;
	}

	/* normalize the rdn attribute name */
	rdn_attr = dsdb_attribute_by_lDAPDisplayName(ar->schema, rdn_name);
	if (rdn_attr == NULL) {
		goto failed;
	}
	rdn_name = rdn_attr->lDAPDisplayName;

	rdn_val = ldb_dn_get_rdn_val(dn);
	if (rdn_val == NULL) {
		goto failed;
	}

	if (ldb_msg_add_empty(msg, rdn_name, LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		goto failed;
	}
	if (ldb_msg_add_value(msg, rdn_name, rdn_val, NULL) != 0) {
		goto failed;
	}
	if (ldb_msg_add_empty(msg, "name", LDB_FLAG_MOD_REPLACE, NULL) != 0) {
		goto failed;
	}
	if (ldb_msg_add_value(msg, "name", rdn_val, NULL) != 0) {
		goto failed;
	}

	/*
	 * We have to mark this as a replicated update otherwise
	 * schema_data may reject a rename in the schema partition
	 */

	ret = dsdb_module_modify(ar->module, msg,
				 DSDB_FLAG_OWN_MODULE|DSDB_FLAG_REPLICATED_UPDATE,
				 req);
	if (ret != LDB_SUCCESS) {
		DEBUG(0,(__location__ ": Failed to modify rDN/name of DN being DRS renamed '%s' - %s",
			 ldb_dn_get_linearized(dn),
			 ldb_errstring(ldb_module_get_ctx(ar->module))));
		return ret;
	}

	talloc_free(msg);

	return LDB_SUCCESS;

failed:
	talloc_free(msg);
	DEBUG(0,(__location__ ": Failed to setup modify rDN/name of DN being DRS renamed '%s'",
		 ldb_dn_get_linearized(dn)));
	return LDB_ERR_OPERATIONS_ERROR;
}