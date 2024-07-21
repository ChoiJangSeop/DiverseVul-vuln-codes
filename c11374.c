static int io_write(struct io_kiocb *req, unsigned int issue_flags)
{
	struct iovec inline_vecs[UIO_FASTIOV], *iovec = inline_vecs;
	struct kiocb *kiocb = &req->rw.kiocb;
	struct iov_iter __iter, *iter = &__iter;
	struct io_async_rw *rw = req->async_data;
	ssize_t ret, ret2, io_size;
	bool force_nonblock = issue_flags & IO_URING_F_NONBLOCK;

	if (rw) {
		iter = &rw->iter;
		iovec = NULL;
	} else {
		ret = io_import_iovec(WRITE, req, &iovec, iter, !force_nonblock);
		if (ret < 0)
			return ret;
	}
	io_size = iov_iter_count(iter);
	req->result = io_size;

	/* Ensure we clear previously set non-block flag */
	if (!force_nonblock)
		kiocb->ki_flags &= ~IOCB_NOWAIT;
	else
		kiocb->ki_flags |= IOCB_NOWAIT;

	/* If the file doesn't support async, just async punt */
	if (force_nonblock && !io_file_supports_async(req, WRITE))
		goto copy_iov;

	/* file path doesn't support NOWAIT for non-direct_IO */
	if (force_nonblock && !(kiocb->ki_flags & IOCB_DIRECT) &&
	    (req->flags & REQ_F_ISREG))
		goto copy_iov;

	ret = rw_verify_area(WRITE, req->file, io_kiocb_ppos(kiocb), io_size);
	if (unlikely(ret))
		goto out_free;

	/*
	 * Open-code file_start_write here to grab freeze protection,
	 * which will be released by another thread in
	 * io_complete_rw().  Fool lockdep by telling it the lock got
	 * released so that it doesn't complain about the held lock when
	 * we return to userspace.
	 */
	if (req->flags & REQ_F_ISREG) {
		sb_start_write(file_inode(req->file)->i_sb);
		__sb_writers_release(file_inode(req->file)->i_sb,
					SB_FREEZE_WRITE);
	}
	kiocb->ki_flags |= IOCB_WRITE;

	if (req->file->f_op->write_iter)
		ret2 = call_write_iter(req->file, kiocb, iter);
	else if (req->file->f_op->write)
		ret2 = loop_rw_iter(WRITE, req, iter);
	else
		ret2 = -EINVAL;

	if (req->flags & REQ_F_REISSUE) {
		req->flags &= ~REQ_F_REISSUE;
		ret2 = -EAGAIN;
	}

	/*
	 * Raw bdev writes will return -EOPNOTSUPP for IOCB_NOWAIT. Just
	 * retry them without IOCB_NOWAIT.
	 */
	if (ret2 == -EOPNOTSUPP && (kiocb->ki_flags & IOCB_NOWAIT))
		ret2 = -EAGAIN;
	/* no retry on NONBLOCK nor RWF_NOWAIT */
	if (ret2 == -EAGAIN && (req->flags & REQ_F_NOWAIT))
		goto done;
	if (!force_nonblock || ret2 != -EAGAIN) {
		/* IOPOLL retry should happen for io-wq threads */
		if ((req->ctx->flags & IORING_SETUP_IOPOLL) && ret2 == -EAGAIN)
			goto copy_iov;
done:
		kiocb_done(kiocb, ret2, issue_flags);
	} else {
copy_iov:
		/* some cases will consume bytes even on error returns */
		iov_iter_revert(iter, io_size - iov_iter_count(iter));
		ret = io_setup_async_rw(req, iovec, inline_vecs, iter, false);
		return ret ?: -EAGAIN;
	}
out_free:
	/* it's reportedly faster than delegating the null check to kfree() */
	if (iovec)
		kfree(iovec);
	return ret;
}