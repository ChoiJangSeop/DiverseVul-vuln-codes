static bool tm_active_with_fp(struct task_struct *tsk)
{
	return MSR_TM_ACTIVE(tsk->thread.regs->msr) &&
		(tsk->thread.ckpt_regs.msr & MSR_FP);
}