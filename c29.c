pfm_setup_buffer_fmt(struct task_struct *task, pfm_context_t *ctx, unsigned int ctx_flags,
		     unsigned int cpu, pfarg_context_t *arg)
{
	pfm_buffer_fmt_t *fmt = NULL;
	unsigned long size = 0UL;
	void *uaddr = NULL;
	void *fmt_arg = NULL;
	int ret = 0;
#define PFM_CTXARG_BUF_ARG(a)	(pfm_buffer_fmt_t *)(a+1)

	/* invoke and lock buffer format, if found */
	fmt = pfm_find_buffer_fmt(arg->ctx_smpl_buf_id);
	if (fmt == NULL) {
		DPRINT(("[%d] cannot find buffer format\n", task->pid));
		return -EINVAL;
	}

	/*
	 * buffer argument MUST be contiguous to pfarg_context_t
	 */
	if (fmt->fmt_arg_size) fmt_arg = PFM_CTXARG_BUF_ARG(arg);

	ret = pfm_buf_fmt_validate(fmt, task, ctx_flags, cpu, fmt_arg);

	DPRINT(("[%d] after validate(0x%x,%d,%p)=%d\n", task->pid, ctx_flags, cpu, fmt_arg, ret));

	if (ret) goto error;

	/* link buffer format and context */
	ctx->ctx_buf_fmt = fmt;

	/*
	 * check if buffer format wants to use perfmon buffer allocation/mapping service
	 */
	ret = pfm_buf_fmt_getsize(fmt, task, ctx_flags, cpu, fmt_arg, &size);
	if (ret) goto error;

	if (size) {
		/*
		 * buffer is always remapped into the caller's address space
		 */
		ret = pfm_smpl_buffer_alloc(current, ctx, size, &uaddr);
		if (ret) goto error;

		/* keep track of user address of buffer */
		arg->ctx_smpl_vaddr = uaddr;
	}
	ret = pfm_buf_fmt_init(fmt, task, ctx->ctx_smpl_hdr, ctx_flags, cpu, fmt_arg);

error:
	return ret;
}