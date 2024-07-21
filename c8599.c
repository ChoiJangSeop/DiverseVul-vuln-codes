static void sfq_perturbation(struct timer_list *t)
{
	struct sfq_sched_data *q = from_timer(q, t, perturb_timer);
	struct Qdisc *sch = q->sch;
	spinlock_t *root_lock = qdisc_lock(qdisc_root_sleeping(sch));

	spin_lock(root_lock);
	q->perturbation = prandom_u32();
	if (!q->filter_list && q->tail)
		sfq_rehash(sch);
	spin_unlock(root_lock);

	if (q->perturb_period)
		mod_timer(&q->perturb_timer, jiffies + q->perturb_period);
}