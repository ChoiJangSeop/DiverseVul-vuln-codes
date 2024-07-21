static int io_submit_sqes(struct io_ring_ctx *ctx, unsigned int nr,
			  struct file *ring_file, int ring_fd)
{
	struct io_submit_state state;
	struct io_kiocb *link = NULL;
	int i, submitted = 0;

	/* if we have a backlog and couldn't flush it all, return BUSY */
	if (test_bit(0, &ctx->sq_check_overflow)) {
		if (!list_empty(&ctx->cq_overflow_list) &&
		    !io_cqring_overflow_flush(ctx, false, NULL, NULL))
			return -EBUSY;
	}

	/* make sure SQ entry isn't read before tail */
	nr = min3(nr, ctx->sq_entries, io_sqring_entries(ctx));

	if (!percpu_ref_tryget_many(&ctx->refs, nr))
		return -EAGAIN;

	io_submit_state_start(&state, ctx, nr);

	ctx->ring_fd = ring_fd;
	ctx->ring_file = ring_file;

	for (i = 0; i < nr; i++) {
		const struct io_uring_sqe *sqe;
		struct io_kiocb *req;
		int err;

		sqe = io_get_sqe(ctx);
		if (unlikely(!sqe)) {
			io_consume_sqe(ctx);
			break;
		}
		req = io_alloc_req(ctx, &state);
		if (unlikely(!req)) {
			if (!submitted)
				submitted = -EAGAIN;
			break;
		}

		err = io_init_req(ctx, req, sqe, &state);
		io_consume_sqe(ctx);
		/* will complete beyond this point, count as submitted */
		submitted++;

		if (unlikely(err)) {
fail_req:
			io_put_req(req);
			io_req_complete(req, err);
			break;
		}

		trace_io_uring_submit_sqe(ctx, req->opcode, req->user_data,
						true, io_async_submit(ctx));
		err = io_submit_sqe(req, sqe, &link, &state.comp);
		if (err)
			goto fail_req;
	}

	if (unlikely(submitted != nr)) {
		int ref_used = (submitted == -EAGAIN) ? 0 : submitted;

		percpu_ref_put_many(&ctx->refs, nr - ref_used);
	}
	if (link)
		io_queue_link_head(link, &state.comp);
	io_submit_state_end(&state);

	 /* Commit SQ ring head once we've consumed and submitted all SQEs */
	io_commit_sqring(ctx);

	return submitted;
}