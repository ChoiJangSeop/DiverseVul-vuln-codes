SYSCALL_DEFINE6(io_uring_enter, unsigned int, fd, u32, to_submit,
		u32, min_complete, u32, flags, const sigset_t __user *, sig,
		size_t, sigsz)
{
	struct io_ring_ctx *ctx;
	long ret = -EBADF;
	int submitted = 0;
	struct fd f;

	io_run_task_work();

	if (flags & ~(IORING_ENTER_GETEVENTS | IORING_ENTER_SQ_WAKEUP))
		return -EINVAL;

	f = fdget(fd);
	if (!f.file)
		return -EBADF;

	ret = -EOPNOTSUPP;
	if (f.file->f_op != &io_uring_fops)
		goto out_fput;

	ret = -ENXIO;
	ctx = f.file->private_data;
	if (!percpu_ref_tryget(&ctx->refs))
		goto out_fput;

	/*
	 * For SQ polling, the thread will do all submissions and completions.
	 * Just return the requested submit count, and wake the thread if
	 * we were asked to.
	 */
	ret = 0;
	if (ctx->flags & IORING_SETUP_SQPOLL) {
		if (!list_empty_careful(&ctx->cq_overflow_list))
			io_cqring_overflow_flush(ctx, false, NULL, NULL);
		if (flags & IORING_ENTER_SQ_WAKEUP)
			wake_up(&ctx->sqo_wait);
		submitted = to_submit;
	} else if (to_submit) {
		mutex_lock(&ctx->uring_lock);
		submitted = io_submit_sqes(ctx, to_submit, f.file, fd);
		mutex_unlock(&ctx->uring_lock);

		if (submitted != to_submit)
			goto out;
	}
	if (flags & IORING_ENTER_GETEVENTS) {
		min_complete = min(min_complete, ctx->cq_entries);

		/*
		 * When SETUP_IOPOLL and SETUP_SQPOLL are both enabled, user
		 * space applications don't need to do io completion events
		 * polling again, they can rely on io_sq_thread to do polling
		 * work, which can reduce cpu usage and uring_lock contention.
		 */
		if (ctx->flags & IORING_SETUP_IOPOLL &&
		    !(ctx->flags & IORING_SETUP_SQPOLL)) {
			ret = io_iopoll_check(ctx, min_complete);
		} else {
			ret = io_cqring_wait(ctx, min_complete, sig, sigsz);
		}
	}

out:
	percpu_ref_put(&ctx->refs);
out_fput:
	fdput(f);
	return submitted ? submitted : ret;
}