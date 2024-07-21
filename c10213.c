
static void io_sq_offload_start(struct io_ring_ctx *ctx)
{
	struct io_sq_data *sqd = ctx->sq_data;

	if (ctx->flags & IORING_SETUP_SQPOLL)
		complete(&sqd->startup);