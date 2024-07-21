static int smk_curacc_on_task(struct task_struct *p, int access,
				const char *caller)
{
	struct smk_audit_info ad;
	struct smack_known *skp = smk_of_task_struct_subj(p);
	int rc;

	smk_ad_init(&ad, caller, LSM_AUDIT_DATA_TASK);
	smk_ad_setfield_u_tsk(&ad, p);
	rc = smk_curacc(skp, access, &ad);
	rc = smk_bu_task(p, access, rc);
	return rc;
}