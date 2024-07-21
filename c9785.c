static void io_req_drop_files(struct io_kiocb *req)
{
	struct io_ring_ctx *ctx = req->ctx;
	unsigned long flags;

	spin_lock_irqsave(&ctx->inflight_lock, flags);
	list_del(&req->inflight_entry);
	if (waitqueue_active(&ctx->inflight_wait))
		wake_up(&ctx->inflight_wait);
	spin_unlock_irqrestore(&ctx->inflight_lock, flags);
	req->flags &= ~REQ_F_INFLIGHT;
	req->work.files = NULL;
}