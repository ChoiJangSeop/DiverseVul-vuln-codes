struct ldb_message *ldb_msg_copy_shallow(TALLOC_CTX *mem_ctx,
					 const struct ldb_message *msg)
{
	struct ldb_message *msg2;
	unsigned int i;

	msg2 = talloc(mem_ctx, struct ldb_message);
	if (msg2 == NULL) return NULL;

	*msg2 = *msg;

	msg2->elements = talloc_array(msg2, struct ldb_message_element,
				      msg2->num_elements);
	if (msg2->elements == NULL) goto failed;

	for (i=0;i<msg2->num_elements;i++) {
		msg2->elements[i] = msg->elements[i];
	}

	return msg2;

failed:
	talloc_free(msg2);
	return NULL;
}