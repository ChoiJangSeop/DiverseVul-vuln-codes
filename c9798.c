static void io_req_free_batch(struct req_batch *rb, struct io_kiocb *req)
{
	if (unlikely(io_is_fallback_req(req))) {
		io_free_req(req);
		return;
	}
	if (req->flags & REQ_F_LINK_HEAD)
		io_queue_next(req);

	if (req->task != rb->task) {
		if (rb->task)
			put_task_struct_many(rb->task, rb->task_refs);
		rb->task = req->task;
		rb->task_refs = 0;
	}
	rb->task_refs++;

	WARN_ON_ONCE(io_dismantle_req(req));
	rb->reqs[rb->to_free++] = req;
	if (unlikely(rb->to_free == ARRAY_SIZE(rb->reqs)))
		__io_req_free_batch_flush(req->ctx, rb);
}