static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	return calc_delta_mine(__sched_period(cfs_rq->nr_running),
			       se->load.weight, &cfs_rq->load);
}