static void __io_req_task_work_add(struct io_kiocb *req,
				   struct io_uring_task *tctx,
				   struct io_wq_work_list *list)
{
	struct io_ring_ctx *ctx = req->ctx;
	struct io_wq_work_node *node;
	unsigned long flags;
	bool running;

	io_drop_inflight_file(req);

	spin_lock_irqsave(&tctx->task_lock, flags);
	wq_list_add_tail(&req->io_task_work.node, list);
	running = tctx->task_running;
	if (!running)
		tctx->task_running = true;
	spin_unlock_irqrestore(&tctx->task_lock, flags);

	/* task_work already pending, we're done */
	if (running)
		return;

	if (ctx->flags & IORING_SETUP_TASKRUN_FLAG)
		atomic_or(IORING_SQ_TASKRUN, &ctx->rings->sq_flags);

	if (likely(!task_work_add(req->task, &tctx->task_work, ctx->notify_method)))
		return;

	spin_lock_irqsave(&tctx->task_lock, flags);
	tctx->task_running = false;
	node = wq_list_merge(&tctx->prio_task_list, &tctx->task_list);
	spin_unlock_irqrestore(&tctx->task_lock, flags);

	while (node) {
		req = container_of(node, struct io_kiocb, io_task_work.node);
		node = node->next;
		if (llist_add(&req->io_task_work.fallback_node,
			      &req->ctx->fallback_llist))
			schedule_delayed_work(&req->ctx->fallback_work, 1);
	}
}