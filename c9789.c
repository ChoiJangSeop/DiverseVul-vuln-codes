static void __io_free_req_finish(struct io_kiocb *req)
{
	struct io_ring_ctx *ctx = req->ctx;

	put_task_struct(req->task);

	if (likely(!io_is_fallback_req(req)))
		kmem_cache_free(req_cachep, req);
	else
		clear_bit_unlock(0, (unsigned long *) &ctx->fallback_req);
	percpu_ref_put(&ctx->refs);
}