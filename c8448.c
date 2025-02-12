static int io_sq_thread(void *data)
{
	struct io_ring_ctx *ctx = data;
	struct mm_struct *cur_mm = NULL;
	mm_segment_t old_fs;
	DEFINE_WAIT(wait);
	unsigned inflight;
	unsigned long timeout;
	int ret;

	complete(&ctx->completions[1]);

	old_fs = get_fs();
	set_fs(USER_DS);

	ret = timeout = inflight = 0;
	while (!kthread_should_park()) {
		unsigned int to_submit;

		if (inflight) {
			unsigned nr_events = 0;

			if (ctx->flags & IORING_SETUP_IOPOLL) {
				/*
				 * inflight is the count of the maximum possible
				 * entries we submitted, but it can be smaller
				 * if we dropped some of them. If we don't have
				 * poll entries available, then we know that we
				 * have nothing left to poll for. Reset the
				 * inflight count to zero in that case.
				 */
				mutex_lock(&ctx->uring_lock);
				if (!list_empty(&ctx->poll_list))
					__io_iopoll_check(ctx, &nr_events, 0);
				else
					inflight = 0;
				mutex_unlock(&ctx->uring_lock);
			} else {
				/*
				 * Normal IO, just pretend everything completed.
				 * We don't have to poll completions for that.
				 */
				nr_events = inflight;
			}

			inflight -= nr_events;
			if (!inflight)
				timeout = jiffies + ctx->sq_thread_idle;
		}

		to_submit = io_sqring_entries(ctx);

		/*
		 * If submit got -EBUSY, flag us as needing the application
		 * to enter the kernel to reap and flush events.
		 */
		if (!to_submit || ret == -EBUSY) {
			/*
			 * We're polling. If we're within the defined idle
			 * period, then let us spin without work before going
			 * to sleep. The exception is if we got EBUSY doing
			 * more IO, we should wait for the application to
			 * reap events and wake us up.
			 */
			if (inflight ||
			    (!time_after(jiffies, timeout) && ret != -EBUSY)) {
				cond_resched();
				continue;
			}

			/*
			 * Drop cur_mm before scheduling, we can't hold it for
			 * long periods (or over schedule()). Do this before
			 * adding ourselves to the waitqueue, as the unuse/drop
			 * may sleep.
			 */
			if (cur_mm) {
				unuse_mm(cur_mm);
				mmput(cur_mm);
				cur_mm = NULL;
			}

			prepare_to_wait(&ctx->sqo_wait, &wait,
						TASK_INTERRUPTIBLE);

			/* Tell userspace we may need a wakeup call */
			ctx->rings->sq_flags |= IORING_SQ_NEED_WAKEUP;
			/* make sure to read SQ tail after writing flags */
			smp_mb();

			to_submit = io_sqring_entries(ctx);
			if (!to_submit || ret == -EBUSY) {
				if (kthread_should_park()) {
					finish_wait(&ctx->sqo_wait, &wait);
					break;
				}
				if (signal_pending(current))
					flush_signals(current);
				schedule();
				finish_wait(&ctx->sqo_wait, &wait);

				ctx->rings->sq_flags &= ~IORING_SQ_NEED_WAKEUP;
				continue;
			}
			finish_wait(&ctx->sqo_wait, &wait);

			ctx->rings->sq_flags &= ~IORING_SQ_NEED_WAKEUP;
		}

		to_submit = min(to_submit, ctx->sq_entries);
		ret = io_submit_sqes(ctx, to_submit, NULL, -1, &cur_mm, true);
		if (ret > 0)
			inflight += ret;
	}

	set_fs(old_fs);
	if (cur_mm) {
		unuse_mm(cur_mm);
		mmput(cur_mm);
	}

	kthread_parkme();

	return 0;
}