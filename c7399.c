static int aio_fsync(struct fsync_iocb *req, const struct iocb *iocb,
		     bool datasync)
{
	if (unlikely(iocb->aio_buf || iocb->aio_offset || iocb->aio_nbytes ||
			iocb->aio_rw_flags))
		return -EINVAL;

	req->file = fget(iocb->aio_fildes);
	if (unlikely(!req->file))
		return -EBADF;
	if (unlikely(!req->file->f_op->fsync)) {
		fput(req->file);
		return -EINVAL;
	}

	req->datasync = datasync;
	INIT_WORK(&req->work, aio_fsync_work);
	schedule_work(&req->work);
	return 0;
}