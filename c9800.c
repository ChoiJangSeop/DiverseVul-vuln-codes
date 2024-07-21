static void __io_cqring_fill_event(struct io_kiocb *req, long res, long cflags)
{
	struct io_ring_ctx *ctx = req->ctx;
	struct io_uring_cqe *cqe;

	trace_io_uring_complete(ctx, req->user_data, res);

	/*
	 * If we can't get a cq entry, userspace overflowed the
	 * submission (by quite a lot). Increment the overflow count in
	 * the ring.
	 */
	cqe = io_get_cqring(ctx);
	if (likely(cqe)) {
		WRITE_ONCE(cqe->user_data, req->user_data);
		WRITE_ONCE(cqe->res, res);
		WRITE_ONCE(cqe->flags, cflags);
	} else if (ctx->cq_overflow_flushed) {
		WRITE_ONCE(ctx->rings->cq_overflow,
				atomic_inc_return(&ctx->cached_cq_overflow));
	} else {
		if (list_empty(&ctx->cq_overflow_list)) {
			set_bit(0, &ctx->sq_check_overflow);
			set_bit(0, &ctx->cq_check_overflow);
			ctx->rings->sq_flags |= IORING_SQ_CQ_OVERFLOW;
		}
		io_clean_op(req);
		req->result = res;
		req->compl.cflags = cflags;
		refcount_inc(&req->refs);
		list_add_tail(&req->compl.list, &ctx->cq_overflow_list);
	}
}