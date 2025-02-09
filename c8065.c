static int restore_fp(struct task_struct *tsk)
{
	if (tsk->thread.load_fp || tm_active_with_fp(tsk)) {
		load_fp_state(&current->thread.fp_state);
		current->thread.load_fp++;
		return 1;
	}
	return 0;
}