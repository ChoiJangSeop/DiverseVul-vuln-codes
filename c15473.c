static int map_modify_do_local(struct map_context *ac)
{
	struct ldb_request *local_req;
	struct ldb_context *ldb;
	int ret;

	ldb = ldb_module_get_ctx(ac->module);

	if (ac->local_dn == NULL) {
		/* No local record present, add it instead */
		/* Add local 'IS_MAPPED' */
		/* TODO: use GUIDs here instead */
		if (ldb_msg_add_empty(ac->local_msg, IS_MAPPED,
					LDB_FLAG_MOD_ADD, NULL) != 0) {
			return LDB_ERR_OPERATIONS_ERROR;
		}
		ret = ldb_msg_add_linearized_dn(ac->local_msg, IS_MAPPED,
						ac->remote_req->op.mod.message->dn);
		if (ret != 0) {
			return LDB_ERR_OPERATIONS_ERROR;
		}

		/* Prepare the local operation */
		ret = ldb_build_add_req(&local_req, ldb, ac,
					ac->local_msg,
					ac->req->controls,
					ac,
					map_op_local_callback,
					ac->req);
		LDB_REQ_SET_LOCATION(local_req);
		if (ret != LDB_SUCCESS) {
			return LDB_ERR_OPERATIONS_ERROR;
		}
	} else {
		/* Prepare the local operation */
		ret = ldb_build_mod_req(&local_req, ldb, ac,
					ac->local_msg,
					ac->req->controls,
					ac,
					map_op_local_callback,
					ac->req);
		LDB_REQ_SET_LOCATION(local_req);
		if (ret != LDB_SUCCESS) {
			return LDB_ERR_OPERATIONS_ERROR;
		}
	}

	return ldb_next_request(ac->module, local_req);
}