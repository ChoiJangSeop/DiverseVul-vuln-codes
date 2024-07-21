int ldb_msg_add_value(struct ldb_message *msg,
		      const char *attr_name,
		      const struct ldb_val *val,
		      struct ldb_message_element **return_el)
{
	struct ldb_message_element *el;
	struct ldb_val *vals;
	int ret;

	el = ldb_msg_find_element(msg, attr_name);
	if (!el) {
		ret = ldb_msg_add_empty(msg, attr_name, 0, &el);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	vals = talloc_realloc(msg->elements, el->values, struct ldb_val,
			      el->num_values+1);
	if (!vals) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	el->values = vals;
	el->values[el->num_values] = *val;
	el->num_values++;

	if (return_el) {
		*return_el = el;
	}

	return LDB_SUCCESS;
}