	__must_hold(&req->ctx->timeout_lock)
{
	struct io_timeout_data *io = req->async_data;

	if (hrtimer_try_to_cancel(&io->timer) != -1) {
		if (status)
			req_set_fail(req);
		atomic_set(&req->ctx->cq_timeouts,
			atomic_read(&req->ctx->cq_timeouts) + 1);
		list_del_init(&req->timeout.list);
		io_req_tw_post_queue(req, status, 0);
	}
}