int samdb_msg_add_addval(struct ldb_context *sam_ldb, TALLOC_CTX *mem_ctx,
			 struct ldb_message *msg, const char *attr_name,
			 const char *value)
{
	struct ldb_message_element *el;
	struct ldb_val val, *vals;
	char *v;
	unsigned int i;
	bool found = false;
	int ret;

	v = talloc_strdup(mem_ctx, value);
	if (v == NULL) {
		return ldb_oom(sam_ldb);
	}

	val.data = (uint8_t *) v;
	val.length = strlen(v);

	if (val.length == 0) {
		/* allow empty strings as non-existent attributes */
		return LDB_SUCCESS;
	}

	for (i = 0; i < msg->num_elements; i++) {
		el = &msg->elements[i];
		if ((ldb_attr_cmp(el->name, attr_name) == 0) &&
		    (LDB_FLAG_MOD_TYPE(el->flags) == LDB_FLAG_MOD_ADD)) {
			found = true;
			break;
		}
	}
	if (!found) {
		ret = ldb_msg_add_empty(msg, attr_name, LDB_FLAG_MOD_ADD,
					&el);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	vals = talloc_realloc(msg->elements, el->values, struct ldb_val,
			      el->num_values + 1);
	if (vals == NULL) {
		return ldb_oom(sam_ldb);
	}
	el->values = vals;
	el->values[el->num_values] = val;
	++(el->num_values);

	return LDB_SUCCESS;
}