static bool io_rw_reissue(struct io_kiocb *req, long res)
{
#ifdef CONFIG_BLOCK
	int ret;

	if ((res != -EAGAIN && res != -EOPNOTSUPP) || io_wq_current_is_worker())
		return false;

	init_task_work(&req->task_work, io_rw_resubmit);
	ret = io_req_task_work_add(req, &req->task_work);
	if (!ret)
		return true;
#endif
	return false;
}