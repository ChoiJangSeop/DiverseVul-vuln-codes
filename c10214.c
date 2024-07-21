
static void io_disable_sqo_submit(struct io_ring_ctx *ctx)
{
	mutex_lock(&ctx->uring_lock);
	ctx->sqo_dead = 1;
	mutex_unlock(&ctx->uring_lock);

	/* make sure callers enter the ring to get error */
	if (ctx->rings)
		io_ring_set_wakeup_flag(ctx);