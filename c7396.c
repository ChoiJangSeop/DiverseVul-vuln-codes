static int __io_submit_one(struct kioctx *ctx, const struct iocb *iocb,
			   struct iocb __user *user_iocb, bool compat)
{
	struct aio_kiocb *req;
	ssize_t ret;

	/* enforce forwards compatibility on users */
	if (unlikely(iocb->aio_reserved2)) {
		pr_debug("EINVAL: reserve field set\n");
		return -EINVAL;
	}

	/* prevent overflows */
	if (unlikely(
	    (iocb->aio_buf != (unsigned long)iocb->aio_buf) ||
	    (iocb->aio_nbytes != (size_t)iocb->aio_nbytes) ||
	    ((ssize_t)iocb->aio_nbytes < 0)
	   )) {
		pr_debug("EINVAL: overflow check\n");
		return -EINVAL;
	}

	if (!get_reqs_available(ctx))
		return -EAGAIN;

	ret = -EAGAIN;
	req = aio_get_req(ctx);
	if (unlikely(!req))
		goto out_put_reqs_available;

	if (iocb->aio_flags & IOCB_FLAG_RESFD) {
		/*
		 * If the IOCB_FLAG_RESFD flag of aio_flags is set, get an
		 * instance of the file* now. The file descriptor must be
		 * an eventfd() fd, and will be signaled for each completed
		 * event using the eventfd_signal() function.
		 */
		req->ki_eventfd = eventfd_ctx_fdget((int) iocb->aio_resfd);
		if (IS_ERR(req->ki_eventfd)) {
			ret = PTR_ERR(req->ki_eventfd);
			req->ki_eventfd = NULL;
			goto out_put_req;
		}
	}

	ret = put_user(KIOCB_KEY, &user_iocb->aio_key);
	if (unlikely(ret)) {
		pr_debug("EFAULT: aio_key\n");
		goto out_put_req;
	}

	req->ki_user_iocb = user_iocb;
	req->ki_user_data = iocb->aio_data;

	switch (iocb->aio_lio_opcode) {
	case IOCB_CMD_PREAD:
		ret = aio_read(&req->rw, iocb, false, compat);
		break;
	case IOCB_CMD_PWRITE:
		ret = aio_write(&req->rw, iocb, false, compat);
		break;
	case IOCB_CMD_PREADV:
		ret = aio_read(&req->rw, iocb, true, compat);
		break;
	case IOCB_CMD_PWRITEV:
		ret = aio_write(&req->rw, iocb, true, compat);
		break;
	case IOCB_CMD_FSYNC:
		ret = aio_fsync(&req->fsync, iocb, false);
		break;
	case IOCB_CMD_FDSYNC:
		ret = aio_fsync(&req->fsync, iocb, true);
		break;
	case IOCB_CMD_POLL:
		ret = aio_poll(req, iocb);
		break;
	default:
		pr_debug("invalid aio operation %d\n", iocb->aio_lio_opcode);
		ret = -EINVAL;
		break;
	}

	/*
	 * If ret is 0, we'd either done aio_complete() ourselves or have
	 * arranged for that to be done asynchronously.  Anything non-zero
	 * means that we need to destroy req ourselves.
	 */
	if (ret)
		goto out_put_req;
	return 0;
out_put_req:
	if (req->ki_eventfd)
		eventfd_ctx_put(req->ki_eventfd);
	iocb_put(req);
out_put_reqs_available:
	put_reqs_available(ctx, 1);
	return ret;
}