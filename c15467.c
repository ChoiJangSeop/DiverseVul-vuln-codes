static int add_time_element(struct ldb_message *msg, const char *attr, time_t t)
{
	struct ldb_message_element *el;
	char *s;
	int ret;

	if (ldb_msg_find_element(msg, attr) != NULL) {
		return LDB_SUCCESS;
	}

	s = ldb_timestring(msg, t);
	if (s == NULL) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ret = ldb_msg_add_string(msg, attr, s);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	el = ldb_msg_find_element(msg, attr);
	/* always set as replace. This works because on add ops, the flag
	   is ignored */
	el->flags = LDB_FLAG_MOD_REPLACE;

	return LDB_SUCCESS;
}