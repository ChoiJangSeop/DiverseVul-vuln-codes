static int ib_prctl_get(struct task_struct *task)
{
	if (!boot_cpu_has_bug(X86_BUG_SPECTRE_V2))
		return PR_SPEC_NOT_AFFECTED;

	switch (spectre_v2_user) {
	case SPECTRE_V2_USER_NONE:
		return PR_SPEC_ENABLE;
	case SPECTRE_V2_USER_PRCTL:
	case SPECTRE_V2_USER_SECCOMP:
		if (task_spec_ib_force_disable(task))
			return PR_SPEC_PRCTL | PR_SPEC_FORCE_DISABLE;
		if (task_spec_ib_disable(task))
			return PR_SPEC_PRCTL | PR_SPEC_DISABLE;
		return PR_SPEC_PRCTL | PR_SPEC_ENABLE;
	case SPECTRE_V2_USER_STRICT:
	case SPECTRE_V2_USER_STRICT_PREFERRED:
		return PR_SPEC_DISABLE;
	default:
		return PR_SPEC_NOT_AFFECTED;
	}
}