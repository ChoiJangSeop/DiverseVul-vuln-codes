static u64 sched_vslice_add(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	return __sched_vslice(cfs_rq->load.weight + se->load.weight,
			cfs_rq->nr_running + 1);
}