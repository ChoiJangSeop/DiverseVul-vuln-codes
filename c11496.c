	__must_hold(&req->ctx->timeout_lock)
{
	struct io_kiocb *link = req->link;

	if (link && link->opcode == IORING_OP_LINK_TIMEOUT) {
		struct io_timeout_data *io = link->async_data;

		io_remove_next_linked(req);
		link->timeout.head = NULL;
		if (hrtimer_try_to_cancel(&io->timer) != -1) {
			list_del(&link->timeout.list);
			return link;
		}
	}
	return NULL;
}