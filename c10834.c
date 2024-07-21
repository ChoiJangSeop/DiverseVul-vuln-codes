static int smack_getprocattr(struct task_struct *p, char *name, char **value)
{
	struct smack_known *skp = smk_of_task_struct_subj(p);
	char *cp;
	int slen;

	if (strcmp(name, "current") != 0)
		return -EINVAL;

	cp = kstrdup(skp->smk_known, GFP_KERNEL);
	if (cp == NULL)
		return -ENOMEM;

	slen = strlen(cp);
	*value = cp;
	return slen;
}