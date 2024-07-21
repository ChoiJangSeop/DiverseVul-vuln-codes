static u64 sched_vslice_add(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	unsigned long nr_running = cfs_rq->nr_running;
	unsigned long weight;
	u64 vslice;

	if (!se->on_rq)
		nr_running++;

	vslice = __sched_period(nr_running);

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);

		weight = cfs_rq->load.weight;
		if (!se->on_rq)
			weight += se->load.weight;

		vslice *= NICE_0_LOAD;
		do_div(vslice, weight);
	}

	return vslice;
}