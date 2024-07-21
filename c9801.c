static int io_grab_files(struct io_kiocb *req)
{
	int ret = -EBADF;
	struct io_ring_ctx *ctx = req->ctx;

	io_req_init_async(req);

	if (req->work.files || (req->flags & REQ_F_NO_FILE_TABLE))
		return 0;
	if (!ctx->ring_file)
		return -EBADF;

	rcu_read_lock();
	spin_lock_irq(&ctx->inflight_lock);
	/*
	 * We use the f_ops->flush() handler to ensure that we can flush
	 * out work accessing these files if the fd is closed. Check if
	 * the fd has changed since we started down this path, and disallow
	 * this operation if it has.
	 */
	if (fcheck(ctx->ring_fd) == ctx->ring_file) {
		list_add(&req->inflight_entry, &ctx->inflight_list);
		req->flags |= REQ_F_INFLIGHT;
		req->work.files = current->files;
		ret = 0;
	}
	spin_unlock_irq(&ctx->inflight_lock);
	rcu_read_unlock();

	return ret;
}