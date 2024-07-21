static void blk_mq_sched_tags_teardown(struct request_queue *q)
{
	struct blk_mq_tag_set *set = q->tag_set;
	struct blk_mq_hw_ctx *hctx;
	int i;

	queue_for_each_hw_ctx(q, hctx, i)
		blk_mq_sched_free_tags(set, hctx, i);
}