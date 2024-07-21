static bool io_poll_disarm(struct io_kiocb *req)
	__must_hold(&ctx->completion_lock)
{
	if (!io_poll_get_ownership(req))
		return false;
	io_poll_remove_entries(req);
	hash_del(&req->hash_node);
	return true;