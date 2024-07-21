place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
	u64 vruntime;

	if (first_fair(cfs_rq)) {
		vruntime = min_vruntime(cfs_rq->min_vruntime,
				__pick_next_entity(cfs_rq)->vruntime);
	} else
		vruntime = cfs_rq->min_vruntime;

	/*
	 * The 'current' period is already promised to the current tasks,
	 * however the extra weight of the new task will slow them down a
	 * little, place the new task so that it fits in the slot that
	 * stays open at the end.
	 */
	if (initial && sched_feat(START_DEBIT))
		vruntime += sched_vslice_add(cfs_rq, se);

	if (!initial) {
		/* sleeps upto a single latency don't count. */
		if (sched_feat(NEW_FAIR_SLEEPERS)) {
			if (sched_feat(NORMALIZED_SLEEPER))
				vruntime -= calc_delta_fair(sysctl_sched_latency,
						&cfs_rq->load);
			else
				vruntime -= sysctl_sched_latency;
		}

		/* ensure we never gain time by being placed backwards. */
		vruntime = max_vruntime(se->vruntime, vruntime);
	}

	se->vruntime = vruntime;
}