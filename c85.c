__update_curr(struct cfs_rq *cfs_rq, struct sched_entity *curr,
	      unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;

	schedstat_set(curr->exec_max, max((u64)delta_exec, curr->exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq, exec_clock, delta_exec);
	delta_exec_weighted = delta_exec;
	if (unlikely(curr->load.weight != NICE_0_LOAD)) {
		delta_exec_weighted = calc_delta_fair(delta_exec_weighted,
							&curr->load);
	}
	curr->vruntime += delta_exec_weighted;
}