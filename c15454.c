int dsdb_user_obj_set_primary_group_id(struct ldb_context *ldb, struct ldb_message *usr_obj,
				       uint32_t user_account_control, uint32_t *group_rid_p)
{
	int ret;
	uint32_t rid;
	struct ldb_message_element *el;

	rid = ds_uf2prim_group_rid(user_account_control);

	ret = samdb_msg_add_uint(ldb, usr_obj, usr_obj,
				 "primaryGroupID", rid);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	el = ldb_msg_find_element(usr_obj, "primaryGroupID");
	el->flags = LDB_FLAG_MOD_REPLACE;

	if (group_rid_p) {
		*group_rid_p = rid;
	}

	return LDB_SUCCESS;
}