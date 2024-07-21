static void io_req_free_batch_finish(struct io_ring_ctx *ctx,
				     struct req_batch *rb)
{
	if (rb->to_free)
		__io_req_free_batch_flush(ctx, rb);
	if (rb->task) {
		put_task_struct_many(rb->task, rb->task_refs);
		rb->task = NULL;
	}
}