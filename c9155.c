void arch_seccomp_spec_mitigate(struct task_struct *task)
{
	if (ssb_mode == SPEC_STORE_BYPASS_SECCOMP)
		ssb_prctl_set(task, PR_SPEC_FORCE_DISABLE);
	if (spectre_v2_user == SPECTRE_V2_USER_SECCOMP)
		ib_prctl_set(task, PR_SPEC_FORCE_DISABLE);
}