static void aio_fsync_work(struct work_struct *work)
{
	struct fsync_iocb *req = container_of(work, struct fsync_iocb, work);
	int ret;

	ret = vfs_fsync(req->file, req->datasync);
	fput(req->file);
	aio_complete(container_of(req, struct aio_kiocb, fsync), ret, 0);
}