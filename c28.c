pfm_context_create(pfm_context_t *ctx, void *arg, int count, struct pt_regs *regs)
{
	pfarg_context_t *req = (pfarg_context_t *)arg;
	struct file *filp;
	int ctx_flags;
	int ret;

	/* let's check the arguments first */
	ret = pfarg_is_sane(current, req);
	if (ret < 0) return ret;

	ctx_flags = req->ctx_flags;

	ret = -ENOMEM;

	ctx = pfm_context_alloc();
	if (!ctx) goto error;

	ret = pfm_alloc_fd(&filp);
	if (ret < 0) goto error_file;

	req->ctx_fd = ctx->ctx_fd = ret;

	/*
	 * attach context to file
	 */
	filp->private_data = ctx;

	/*
	 * does the user want to sample?
	 */
	if (pfm_uuid_cmp(req->ctx_smpl_buf_id, pfm_null_uuid)) {
		ret = pfm_setup_buffer_fmt(current, ctx, ctx_flags, 0, req);
		if (ret) goto buffer_error;
	}

	/*
	 * init context protection lock
	 */
	spin_lock_init(&ctx->ctx_lock);

	/*
	 * context is unloaded
	 */
	ctx->ctx_state = PFM_CTX_UNLOADED;

	/*
	 * initialization of context's flags
	 */
	ctx->ctx_fl_block       = (ctx_flags & PFM_FL_NOTIFY_BLOCK) ? 1 : 0;
	ctx->ctx_fl_system      = (ctx_flags & PFM_FL_SYSTEM_WIDE) ? 1: 0;
	ctx->ctx_fl_is_sampling = ctx->ctx_buf_fmt ? 1 : 0; /* assume record() is defined */
	ctx->ctx_fl_no_msg      = (ctx_flags & PFM_FL_OVFL_NO_MSG) ? 1: 0;
	/*
	 * will move to set properties
	 * ctx->ctx_fl_excl_idle   = (ctx_flags & PFM_FL_EXCL_IDLE) ? 1: 0;
	 */

	/*
	 * init restart semaphore to locked
	 */
	init_completion(&ctx->ctx_restart_done);

	/*
	 * activation is used in SMP only
	 */
	ctx->ctx_last_activation = PFM_INVALID_ACTIVATION;
	SET_LAST_CPU(ctx, -1);

	/*
	 * initialize notification message queue
	 */
	ctx->ctx_msgq_head = ctx->ctx_msgq_tail = 0;
	init_waitqueue_head(&ctx->ctx_msgq_wait);
	init_waitqueue_head(&ctx->ctx_zombieq);

	DPRINT(("ctx=%p flags=0x%x system=%d notify_block=%d excl_idle=%d no_msg=%d ctx_fd=%d \n",
		ctx,
		ctx_flags,
		ctx->ctx_fl_system,
		ctx->ctx_fl_block,
		ctx->ctx_fl_excl_idle,
		ctx->ctx_fl_no_msg,
		ctx->ctx_fd));

	/*
	 * initialize soft PMU state
	 */
	pfm_reset_pmu_state(ctx);

	return 0;

buffer_error:
	pfm_free_fd(ctx->ctx_fd, filp);

	if (ctx->ctx_buf_fmt) {
		pfm_buf_fmt_exit(ctx->ctx_buf_fmt, current, NULL, regs);
	}
error_file:
	pfm_context_free(ctx);

error:
	return ret;
}