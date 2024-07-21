static void io_req_task_queue(struct io_kiocb *req)
{
	int ret;

	init_task_work(&req->task_work, io_req_task_submit);

	ret = io_req_task_work_add(req, &req->task_work);
	if (unlikely(ret)) {
		struct task_struct *tsk;

		init_task_work(&req->task_work, io_req_task_cancel);
		tsk = io_wq_get_task(req->ctx->io_wq);
		task_work_add(tsk, &req->task_work, 0);
		wake_up_process(tsk);
	}
}