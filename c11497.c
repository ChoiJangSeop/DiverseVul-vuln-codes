
static s64 tctx_inflight(struct io_uring_task *tctx, bool tracked)
{
	if (tracked)
		return 0;
	return percpu_counter_sum(&tctx->inflight);