int dsdb_user_obj_set_account_type(struct ldb_context *ldb, struct ldb_message *usr_obj,
				   uint32_t user_account_control, uint32_t *account_type_p)
{
	int ret;
	uint32_t account_type;
	struct ldb_message_element *el;

	account_type = ds_uf2atype(user_account_control);
	if (account_type == 0) {
		ldb_set_errstring(ldb, "dsdb: Unrecognized account type!");
		return LDB_ERR_UNWILLING_TO_PERFORM;
	}
	ret = samdb_msg_add_uint(ldb, usr_obj, usr_obj,
				 "sAMAccountType",
				 account_type);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	el = ldb_msg_find_element(usr_obj, "sAMAccountType");
	el->flags = LDB_FLAG_MOD_REPLACE;

	if (account_type_p) {
		*account_type_p = account_type;
	}

	return LDB_SUCCESS;
}