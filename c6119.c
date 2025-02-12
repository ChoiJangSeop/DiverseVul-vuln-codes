static void flush_tmregs_to_thread(struct task_struct *tsk)
{
	/*
	 * If task is not current, it will have been flushed already to
	 * it's thread_struct during __switch_to().
	 *
	 * A reclaim flushes ALL the state or if not in TM save TM SPRs
	 * in the appropriate thread structures from live.
	 */

	if (tsk != current)
		return;

	if (MSR_TM_SUSPENDED(mfmsr())) {
		tm_reclaim_current(TM_CAUSE_SIGNAL);
	} else {
		tm_enable();
		tm_save_sprs(&(tsk->thread));
	}
}