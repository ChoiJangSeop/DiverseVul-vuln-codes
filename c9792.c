static int io_init_req(struct io_ring_ctx *ctx, struct io_kiocb *req,
		       const struct io_uring_sqe *sqe,
		       struct io_submit_state *state)
{
	unsigned int sqe_flags;
	int id;

	req->opcode = READ_ONCE(sqe->opcode);
	req->user_data = READ_ONCE(sqe->user_data);
	req->io = NULL;
	req->file = NULL;
	req->ctx = ctx;
	req->flags = 0;
	/* one is dropped after submission, the other at completion */
	refcount_set(&req->refs, 2);
	req->task = current;
	get_task_struct(req->task);
	req->result = 0;

	if (unlikely(req->opcode >= IORING_OP_LAST))
		return -EINVAL;

	if (unlikely(io_sq_thread_acquire_mm(ctx, req)))
		return -EFAULT;

	sqe_flags = READ_ONCE(sqe->flags);
	/* enforce forwards compatibility on users */
	if (unlikely(sqe_flags & ~SQE_VALID_FLAGS))
		return -EINVAL;

	if ((sqe_flags & IOSQE_BUFFER_SELECT) &&
	    !io_op_defs[req->opcode].buffer_select)
		return -EOPNOTSUPP;

	id = READ_ONCE(sqe->personality);
	if (id) {
		io_req_init_async(req);
		req->work.creds = idr_find(&ctx->personality_idr, id);
		if (unlikely(!req->work.creds))
			return -EINVAL;
		get_cred(req->work.creds);
	}

	/* same numerical values with corresponding REQ_F_*, safe to copy */
	req->flags |= sqe_flags;

	if (!io_op_defs[req->opcode].needs_file)
		return 0;

	return io_req_set_file(state, req, READ_ONCE(sqe->fd));
}