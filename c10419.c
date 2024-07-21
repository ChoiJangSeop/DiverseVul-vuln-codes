int blk_mq_init_sched(struct request_queue *q, struct elevator_type *e)
{
	struct blk_mq_hw_ctx *hctx;
	struct elevator_queue *eq;
	unsigned int i;
	int ret;

	if (!e) {
		q->elevator = NULL;
		q->nr_requests = q->tag_set->queue_depth;
		return 0;
	}

	/*
	 * Default to double of smaller one between hw queue_depth and 128,
	 * since we don't split into sync/async like the old code did.
	 * Additionally, this is a per-hw queue depth.
	 */
	q->nr_requests = 2 * min_t(unsigned int, q->tag_set->queue_depth,
				   BLKDEV_MAX_RQ);

	queue_for_each_hw_ctx(q, hctx, i) {
		ret = blk_mq_sched_alloc_tags(q, hctx, i);
		if (ret)
			goto err;
	}

	ret = e->ops.init_sched(q, e);
	if (ret)
		goto err;

	blk_mq_debugfs_register_sched(q);

	queue_for_each_hw_ctx(q, hctx, i) {
		if (e->ops.init_hctx) {
			ret = e->ops.init_hctx(hctx, i);
			if (ret) {
				eq = q->elevator;
				blk_mq_exit_sched(q, eq);
				kobject_put(&eq->kobj);
				return ret;
			}
		}
		blk_mq_debugfs_register_sched_hctx(q, hctx);
	}

	return 0;

err:
	blk_mq_sched_tags_teardown(q);
	q->elevator = NULL;
	return ret;
}