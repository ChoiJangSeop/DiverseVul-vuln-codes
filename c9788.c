static int io_sq_thread(void *data)
{
	struct io_ring_ctx *ctx = data;
	const struct cred *old_cred;
	DEFINE_WAIT(wait);
	unsigned long timeout;
	int ret = 0;

	complete(&ctx->sq_thread_comp);

	old_cred = override_creds(ctx->creds);

	timeout = jiffies + ctx->sq_thread_idle;
	while (!kthread_should_park()) {
		unsigned int to_submit;

		if (!list_empty(&ctx->iopoll_list)) {
			unsigned nr_events = 0;

			mutex_lock(&ctx->uring_lock);
			if (!list_empty(&ctx->iopoll_list) && !need_resched())
				io_do_iopoll(ctx, &nr_events, 0);
			else
				timeout = jiffies + ctx->sq_thread_idle;
			mutex_unlock(&ctx->uring_lock);
		}

		to_submit = io_sqring_entries(ctx);

		/*
		 * If submit got -EBUSY, flag us as needing the application
		 * to enter the kernel to reap and flush events.
		 */
		if (!to_submit || ret == -EBUSY || need_resched()) {
			/*
			 * Drop cur_mm before scheduling, we can't hold it for
			 * long periods (or over schedule()). Do this before
			 * adding ourselves to the waitqueue, as the unuse/drop
			 * may sleep.
			 */
			io_sq_thread_drop_mm();

			/*
			 * We're polling. If we're within the defined idle
			 * period, then let us spin without work before going
			 * to sleep. The exception is if we got EBUSY doing
			 * more IO, we should wait for the application to
			 * reap events and wake us up.
			 */
			if (!list_empty(&ctx->iopoll_list) || need_resched() ||
			    (!time_after(jiffies, timeout) && ret != -EBUSY &&
			    !percpu_ref_is_dying(&ctx->refs))) {
				io_run_task_work();
				cond_resched();
				continue;
			}

			prepare_to_wait(&ctx->sqo_wait, &wait,
						TASK_INTERRUPTIBLE);

			/*
			 * While doing polled IO, before going to sleep, we need
			 * to check if there are new reqs added to iopoll_list,
			 * it is because reqs may have been punted to io worker
			 * and will be added to iopoll_list later, hence check
			 * the iopoll_list again.
			 */
			if ((ctx->flags & IORING_SETUP_IOPOLL) &&
			    !list_empty_careful(&ctx->iopoll_list)) {
				finish_wait(&ctx->sqo_wait, &wait);
				continue;
			}

			io_ring_set_wakeup_flag(ctx);

			to_submit = io_sqring_entries(ctx);
			if (!to_submit || ret == -EBUSY) {
				if (kthread_should_park()) {
					finish_wait(&ctx->sqo_wait, &wait);
					break;
				}
				if (io_run_task_work()) {
					finish_wait(&ctx->sqo_wait, &wait);
					io_ring_clear_wakeup_flag(ctx);
					continue;
				}
				if (signal_pending(current))
					flush_signals(current);
				schedule();
				finish_wait(&ctx->sqo_wait, &wait);

				io_ring_clear_wakeup_flag(ctx);
				ret = 0;
				continue;
			}
			finish_wait(&ctx->sqo_wait, &wait);

			io_ring_clear_wakeup_flag(ctx);
		}

		mutex_lock(&ctx->uring_lock);
		if (likely(!percpu_ref_is_dying(&ctx->refs)))
			ret = io_submit_sqes(ctx, to_submit, NULL, -1);
		mutex_unlock(&ctx->uring_lock);
		timeout = jiffies + ctx->sq_thread_idle;
	}

	io_run_task_work();

	io_sq_thread_drop_mm();
	revert_creds(old_cred);

	kthread_parkme();

	return 0;
}