	__must_hold(&ctx->completion_lock)
{
	u32 seq = ctx->cached_cq_tail - atomic_read(&ctx->cq_timeouts);

	spin_lock_irq(&ctx->timeout_lock);
	while (!list_empty(&ctx->timeout_list)) {
		u32 events_needed, events_got;
		struct io_kiocb *req = list_first_entry(&ctx->timeout_list,
						struct io_kiocb, timeout.list);

		if (io_is_timeout_noseq(req))
			break;

		/*
		 * Since seq can easily wrap around over time, subtract
		 * the last seq at which timeouts were flushed before comparing.
		 * Assuming not more than 2^31-1 events have happened since,
		 * these subtractions won't have wrapped, so we can check if
		 * target is in [last_seq, current_seq] by comparing the two.
		 */
		events_needed = req->timeout.target_seq - ctx->cq_last_tm_flush;
		events_got = seq - ctx->cq_last_tm_flush;
		if (events_got < events_needed)
			break;

		list_del_init(&req->timeout.list);
		io_kill_timeout(req, 0);
	}
	ctx->cq_last_tm_flush = seq;
	spin_unlock_irq(&ctx->timeout_lock);
}