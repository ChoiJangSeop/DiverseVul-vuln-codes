	__must_hold(&req->ctx->timeout_lock)
{
	if (task && head->task != task)
		return false;
	return cancel_all;
}