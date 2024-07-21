static bool io_uring_cancel_files(struct io_ring_ctx *ctx,
				  struct files_struct *files)
{
	if (list_empty_careful(&ctx->inflight_list))
		return false;

	io_cancel_defer_files(ctx, files);
	/* cancel all at once, should be faster than doing it one by one*/
	io_wq_cancel_cb(ctx->io_wq, io_wq_files_match, files, true);

	while (!list_empty_careful(&ctx->inflight_list)) {
		struct io_kiocb *cancel_req = NULL, *req;
		DEFINE_WAIT(wait);

		spin_lock_irq(&ctx->inflight_lock);
		list_for_each_entry(req, &ctx->inflight_list, inflight_entry) {
			if (req->work.files != files)
				continue;
			/* req is being completed, ignore */
			if (!refcount_inc_not_zero(&req->refs))
				continue;
			cancel_req = req;
			break;
		}
		if (cancel_req)
			prepare_to_wait(&ctx->inflight_wait, &wait,
						TASK_UNINTERRUPTIBLE);
		spin_unlock_irq(&ctx->inflight_lock);

		/* We need to keep going until we don't find a matching req */
		if (!cancel_req)
			break;
		/* cancel this request, or head link requests */
		io_attempt_cancel(ctx, cancel_req);
		io_put_req(cancel_req);
		/* cancellations _may_ trigger task work */
		io_run_task_work();
		schedule();
		finish_wait(&ctx->inflight_wait, &wait);
	}

	return true;
}