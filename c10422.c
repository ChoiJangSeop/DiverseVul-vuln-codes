static void blk_exit_queue(struct request_queue *q)
{
	/*
	 * Since the I/O scheduler exit code may access cgroup information,
	 * perform I/O scheduler exit before disassociating from the block
	 * cgroup controller.
	 */
	if (q->elevator) {
		ioc_clear_queue(q);
		elevator_exit(q, q->elevator);
		q->elevator = NULL;
	}

	/*
	 * Remove all references to @q from the block cgroup controller before
	 * restoring @q->queue_lock to avoid that restoring this pointer causes
	 * e.g. blkcg_print_blkgs() to crash.
	 */
	blkcg_exit_queue(q);

	/*
	 * Since the cgroup code may dereference the @q->backing_dev_info
	 * pointer, only decrease its reference count after having removed the
	 * association with the block cgroup controller.
	 */
	bdi_put(q->backing_dev_info);
}