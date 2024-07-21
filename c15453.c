static int add_uint64_element(struct ldb_context *ldb, struct ldb_message *msg,
			      const char *attr, uint64_t v)
{
	struct ldb_message_element *el;
	int ret;

	if (ldb_msg_find_element(msg, attr) != NULL) {
		return LDB_SUCCESS;
	}

	ret = samdb_msg_add_uint64(ldb, msg, msg, attr, v);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	el = ldb_msg_find_element(msg, attr);
	/* always set as replace. This works because on add ops, the flag
	   is ignored */
	el->flags = LDB_FLAG_MOD_REPLACE;

	return LDB_SUCCESS;
}