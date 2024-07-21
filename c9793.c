static int io_uring_flush(struct file *file, void *data)
{
	struct io_ring_ctx *ctx = file->private_data;

	io_uring_cancel_files(ctx, data);

	/*
	 * If the task is going away, cancel work it may have pending
	 */
	if (fatal_signal_pending(current) || (current->flags & PF_EXITING))
		io_wq_cancel_cb(ctx->io_wq, io_cancel_task_cb, current, true);

	return 0;
}