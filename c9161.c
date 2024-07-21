static int ib_prctl_set(struct task_struct *task, unsigned long ctrl)
{
	switch (ctrl) {
	case PR_SPEC_ENABLE:
		if (spectre_v2_user_ibpb == SPECTRE_V2_USER_NONE &&
		    spectre_v2_user_stibp == SPECTRE_V2_USER_NONE)
			return 0;
		/*
		 * Indirect branch speculation is always disabled in strict
		 * mode.
		 */
		if (spectre_v2_user_ibpb == SPECTRE_V2_USER_STRICT ||
		    spectre_v2_user_stibp == SPECTRE_V2_USER_STRICT ||
		    spectre_v2_user_stibp == SPECTRE_V2_USER_STRICT_PREFERRED)
			return -EPERM;
		task_clear_spec_ib_disable(task);
		task_update_spec_tif(task);
		break;
	case PR_SPEC_DISABLE:
	case PR_SPEC_FORCE_DISABLE:
		/*
		 * Indirect branch speculation is always allowed when
		 * mitigation is force disabled.
		 */
		if (spectre_v2_user_ibpb == SPECTRE_V2_USER_NONE &&
		    spectre_v2_user_stibp == SPECTRE_V2_USER_NONE)
			return -EPERM;
		if (spectre_v2_user_ibpb == SPECTRE_V2_USER_STRICT ||
		    spectre_v2_user_stibp == SPECTRE_V2_USER_STRICT ||
		    spectre_v2_user_stibp == SPECTRE_V2_USER_STRICT_PREFERRED)
			return 0;
		task_set_spec_ib_disable(task);
		if (ctrl == PR_SPEC_FORCE_DISABLE)
			task_set_spec_ib_force_disable(task);
		task_update_spec_tif(task);
		break;
	default:
		return -ERANGE;
	}
	return 0;
}