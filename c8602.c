static int sfq_init(struct Qdisc *sch, struct nlattr *opt,
		    struct netlink_ext_ack *extack)
{
	struct sfq_sched_data *q = qdisc_priv(sch);
	int i;
	int err;

	q->sch = sch;
	timer_setup(&q->perturb_timer, sfq_perturbation, TIMER_DEFERRABLE);

	err = tcf_block_get(&q->block, &q->filter_list, sch, extack);
	if (err)
		return err;

	for (i = 0; i < SFQ_MAX_DEPTH + 1; i++) {
		q->dep[i].next = i + SFQ_MAX_FLOWS;
		q->dep[i].prev = i + SFQ_MAX_FLOWS;
	}

	q->limit = SFQ_MAX_DEPTH;
	q->maxdepth = SFQ_MAX_DEPTH;
	q->cur_depth = 0;
	q->tail = NULL;
	q->divisor = SFQ_DEFAULT_HASH_DIVISOR;
	q->maxflows = SFQ_DEFAULT_FLOWS;
	q->quantum = psched_mtu(qdisc_dev(sch));
	q->scaled_quantum = SFQ_ALLOT_SIZE(q->quantum);
	q->perturb_period = 0;
	q->perturbation = prandom_u32();

	if (opt) {
		int err = sfq_change(sch, opt);
		if (err)
			return err;
	}

	q->ht = sfq_alloc(sizeof(q->ht[0]) * q->divisor);
	q->slots = sfq_alloc(sizeof(q->slots[0]) * q->maxflows);
	if (!q->ht || !q->slots) {
		/* Note: sfq_destroy() will be called by our caller */
		return -ENOMEM;
	}

	for (i = 0; i < q->divisor; i++)
		q->ht[i] = SFQ_EMPTY_SLOT;

	for (i = 0; i < q->maxflows; i++) {
		slot_queue_init(&q->slots[i]);
		sfq_link(q, i);
	}
	if (q->limit >= 1)
		sch->flags |= TCQ_F_CAN_BYPASS;
	else
		sch->flags &= ~TCQ_F_CAN_BYPASS;
	return 0;
}