static void io_rw_resubmit(struct callback_head *cb)
{
	struct io_kiocb *req = container_of(cb, struct io_kiocb, task_work);
	struct io_ring_ctx *ctx = req->ctx;
	int err;

	err = io_sq_thread_acquire_mm(ctx, req);

	if (io_resubmit_prep(req, err)) {
		refcount_inc(&req->refs);
		io_queue_async_work(req);
	}
}