
static int io_timeout_prep(struct io_kiocb *req, const struct io_uring_sqe *sqe,
			   bool is_timeout_link)
{
	struct io_timeout_data *data;
	unsigned flags;
	u32 off = READ_ONCE(sqe->off);

	if (unlikely(req->ctx->flags & IORING_SETUP_IOPOLL))
		return -EINVAL;
	if (sqe->ioprio || sqe->buf_index || sqe->len != 1 ||
	    sqe->splice_fd_in)
		return -EINVAL;
	if (off && is_timeout_link)
		return -EINVAL;
	flags = READ_ONCE(sqe->timeout_flags);
	if (flags & ~(IORING_TIMEOUT_ABS | IORING_TIMEOUT_CLOCK_MASK |
		      IORING_TIMEOUT_ETIME_SUCCESS))
		return -EINVAL;
	/* more than one clock specified is invalid, obviously */
	if (hweight32(flags & IORING_TIMEOUT_CLOCK_MASK) > 1)
		return -EINVAL;

	INIT_LIST_HEAD(&req->timeout.list);
	req->timeout.off = off;
	if (unlikely(off && !req->ctx->off_timeout_used))
		req->ctx->off_timeout_used = true;

	if (WARN_ON_ONCE(req_has_async_data(req)))
		return -EFAULT;
	if (io_alloc_async_data(req))
		return -ENOMEM;

	data = req->async_data;
	data->req = req;
	data->flags = flags;

	if (get_timespec64(&data->ts, u64_to_user_ptr(sqe->addr)))
		return -EFAULT;

	if (data->ts.tv_sec < 0 || data->ts.tv_nsec < 0)
		return -EINVAL;

	data->mode = io_translate_timeout_mode(flags);
	hrtimer_init(&data->timer, io_timeout_get_clock(data), data->mode);

	if (is_timeout_link) {
		struct io_submit_link *link = &req->ctx->submit_state.link;

		if (!link->head)
			return -EINVAL;
		if (link->last->opcode == IORING_OP_LINK_TIMEOUT)
			return -EINVAL;
		req->timeout.head = link->last;
		link->last->flags |= REQ_F_ARM_LTIMEOUT;
	}
	return 0;