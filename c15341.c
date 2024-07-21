struct ldb_message *ldb_msg_copy(TALLOC_CTX *mem_ctx,
				 const struct ldb_message *msg)
{
	struct ldb_message *msg2;
	unsigned int i, j;

	msg2 = ldb_msg_copy_shallow(mem_ctx, msg);
	if (msg2 == NULL) return NULL;

	if (msg2->dn != NULL) {
		msg2->dn = ldb_dn_copy(msg2, msg2->dn);
		if (msg2->dn == NULL) goto failed;
	}

	for (i=0;i<msg2->num_elements;i++) {
		struct ldb_message_element *el = &msg2->elements[i];
		struct ldb_val *values = el->values;
		el->name = talloc_strdup(msg2->elements, el->name);
		if (el->name == NULL) goto failed;
		el->values = talloc_array(msg2->elements, struct ldb_val, el->num_values);
		if (el->values == NULL) goto failed;
		for (j=0;j<el->num_values;j++) {
			el->values[j] = ldb_val_dup(el->values, &values[j]);
			if (el->values[j].data == NULL && values[j].length != 0) {
				goto failed;
			}
		}
	}

	return msg2;

failed:
	talloc_free(msg2);
	return NULL;
}