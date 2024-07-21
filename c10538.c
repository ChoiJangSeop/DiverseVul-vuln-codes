static void io_poll_task_func(struct callback_head *cb)
{
	struct io_kiocb *req = container_of(cb, struct io_kiocb, task_work);
	struct io_kiocb *nxt = NULL;

	io_poll_task_handler(req, &nxt);
	if (nxt)
		__io_req_task_submit(nxt);
}