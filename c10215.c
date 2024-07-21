
static int io_register_enable_rings(struct io_ring_ctx *ctx)
{
	if (!(ctx->flags & IORING_SETUP_R_DISABLED))
		return -EBADFD;

	if (ctx->restrictions.registered)
		ctx->restricted = 1;

	ctx->flags &= ~IORING_SETUP_R_DISABLED;

	io_sq_offload_start(ctx);

	return 0;