static void pending_op_free(void *data)
{
	struct pending_op *op = data;

	if (op->owner_queue)
		queue_remove(op->owner_queue, op);

	free(op);
}