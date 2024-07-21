static ssize_t aio_write(struct kiocb *req, const struct iocb *iocb,
			 bool vectored, bool compat)
{
	struct iovec inline_vecs[UIO_FASTIOV], *iovec = inline_vecs;
	struct iov_iter iter;
	struct file *file;
	ssize_t ret;

	ret = aio_prep_rw(req, iocb);
	if (ret)
		return ret;
	file = req->ki_filp;

	ret = -EBADF;
	if (unlikely(!(file->f_mode & FMODE_WRITE)))
		goto out_fput;
	ret = -EINVAL;
	if (unlikely(!file->f_op->write_iter))
		goto out_fput;

	ret = aio_setup_rw(WRITE, iocb, &iovec, vectored, compat, &iter);
	if (ret)
		goto out_fput;
	ret = rw_verify_area(WRITE, file, &req->ki_pos, iov_iter_count(&iter));
	if (!ret) {
		/*
		 * Open-code file_start_write here to grab freeze protection,
		 * which will be released by another thread in
		 * aio_complete_rw().  Fool lockdep by telling it the lock got
		 * released so that it doesn't complain about the held lock when
		 * we return to userspace.
		 */
		if (S_ISREG(file_inode(file)->i_mode)) {
			__sb_start_write(file_inode(file)->i_sb, SB_FREEZE_WRITE, true);
			__sb_writers_release(file_inode(file)->i_sb, SB_FREEZE_WRITE);
		}
		req->ki_flags |= IOCB_WRITE;
		aio_rw_done(req, call_write_iter(file, req, &iter));
	}
	kfree(iovec);
out_fput:
	if (unlikely(ret))
		fput(file);
	return ret;
}