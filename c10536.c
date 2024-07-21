static int io_async_buf_func(struct wait_queue_entry *wait, unsigned mode,
			     int sync, void *arg)
{
	struct wait_page_queue *wpq;
	struct io_kiocb *req = wait->private;
	struct wait_page_key *key = arg;
	int ret;

	wpq = container_of(wait, struct wait_page_queue, wait);

	if (!wake_page_match(wpq, key))
		return 0;

	list_del_init(&wait->entry);

	init_task_work(&req->task_work, io_req_task_submit);
	/* submit ref gets dropped, acquire a new one */
	refcount_inc(&req->refs);
	ret = io_req_task_work_add(req, &req->task_work);
	if (unlikely(ret)) {
		struct task_struct *tsk;

		/* queue just for cancelation */
		init_task_work(&req->task_work, io_req_task_cancel);
		tsk = io_wq_get_task(req->ctx->io_wq);
		task_work_add(tsk, &req->task_work, 0);
		wake_up_process(tsk);
	}
	return 1;
}