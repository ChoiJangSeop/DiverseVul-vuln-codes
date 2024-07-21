static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 slice = __sched_period(cfs_rq->nr_running);

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);

		slice *= se->load.weight;
		do_div(slice, cfs_rq->load.weight);
	}


	return slice;
}