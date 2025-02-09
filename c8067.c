static int restore_altivec(struct task_struct *tsk)
{
	if (cpu_has_feature(CPU_FTR_ALTIVEC) &&
		(tsk->thread.load_vec || tm_active_with_altivec(tsk))) {
		load_vr_state(&tsk->thread.vr_state);
		tsk->thread.used_vr = 1;
		tsk->thread.load_vec++;

		return 1;
	}
	return 0;
}