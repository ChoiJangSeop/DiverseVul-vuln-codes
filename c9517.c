static void cancel_att_send_op(struct att_send_op *op)
{
	if (op->destroy)
		op->destroy(op->user_data);

	op->user_data = NULL;
	op->callback = NULL;
	op->destroy = NULL;
}