static int __io_async_wake(struct io_kiocb *req, struct io_poll_iocb *poll,
			   __poll_t mask, task_work_func_t func)
{
	int ret;

	/* for instances that support it check for an event match first: */
	if (mask && !(mask & poll->events))
		return 0;

	trace_io_uring_task_add(req->ctx, req->opcode, req->user_data, mask);

	list_del_init(&poll->wait.entry);

	req->result = mask;
	init_task_work(&req->task_work, func);
	/*
	 * If this fails, then the task is exiting. When a task exits, the
	 * work gets canceled, so just cancel this request as well instead
	 * of executing it. We can't safely execute it anyway, as we may not
	 * have the needed state needed for it anyway.
	 */
	ret = io_req_task_work_add(req, &req->task_work);
	if (unlikely(ret)) {
		struct task_struct *tsk;

		WRITE_ONCE(poll->canceled, true);
		tsk = io_wq_get_task(req->ctx->io_wq);
		task_work_add(tsk, &req->task_work, 0);
		wake_up_process(tsk);
	}
	return 1;
}