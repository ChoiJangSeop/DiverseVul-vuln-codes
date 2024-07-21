	__must_hold(&req->ctx->completion_lock)
{
	struct io_kiocb *nxt, *link = req->link;
	bool ignore_cqes = req->flags & REQ_F_SKIP_LINK_CQES;

	req->link = NULL;
	while (link) {
		long res = -ECANCELED;

		if (link->flags & REQ_F_FAIL)
			res = link->result;

		nxt = link->link;
		link->link = NULL;

		trace_io_uring_fail_link(req->ctx, req, req->user_data,
					req->opcode, link);

		if (!ignore_cqes) {
			link->flags &= ~REQ_F_CQE_SKIP;
			io_fill_cqe_req(link, res, 0);
		}
		io_put_req_deferred(link);
		link = nxt;
	}
}