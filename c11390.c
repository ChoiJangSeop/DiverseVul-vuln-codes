	__must_hold(&req->ctx->completion_lock)
{
	bool posted = false;

	if (req->flags & REQ_F_ARM_LTIMEOUT) {
		struct io_kiocb *link = req->link;

		req->flags &= ~REQ_F_ARM_LTIMEOUT;
		if (link && link->opcode == IORING_OP_LINK_TIMEOUT) {
			io_remove_next_linked(req);
			/* leave REQ_F_CQE_SKIP to io_fill_cqe_req */
			io_fill_cqe_req(link, -ECANCELED, 0);
			io_put_req_deferred(link);
			posted = true;
		}
	} else if (req->flags & REQ_F_LINK_TIMEOUT) {
		struct io_ring_ctx *ctx = req->ctx;

		spin_lock_irq(&ctx->timeout_lock);
		posted = io_kill_linked_timeout(req);
		spin_unlock_irq(&ctx->timeout_lock);
	}
	if (unlikely((req->flags & REQ_F_FAIL) &&
		     !(req->flags & REQ_F_HARDLINK))) {
		posted |= (req->link != NULL);
		io_fail_links(req);
	}
	return posted;
}