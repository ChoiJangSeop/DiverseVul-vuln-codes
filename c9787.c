static int io_close_prep(struct io_kiocb *req, const struct io_uring_sqe *sqe)
{
	/*
	 * If we queue this for async, it must not be cancellable. That would
	 * leave the 'file' in an undeterminate state, and here need to modify
	 * io_wq_work.flags, so initialize io_wq_work firstly.
	 */
	io_req_init_async(req);
	req->work.flags |= IO_WQ_WORK_NO_CANCEL;

	if (unlikely(req->ctx->flags & (IORING_SETUP_IOPOLL|IORING_SETUP_SQPOLL)))
		return -EINVAL;
	if (sqe->ioprio || sqe->off || sqe->addr || sqe->len ||
	    sqe->rw_flags || sqe->buf_index)
		return -EINVAL;
	if (req->flags & REQ_F_FIXED_FILE)
		return -EBADF;

	req->close.fd = READ_ONCE(sqe->fd);
	if ((req->file && req->file->f_op == &io_uring_fops) ||
	    req->close.fd == req->ctx->ring_fd)
		return -EBADF;

	req->close.put_file = NULL;
	return 0;
}