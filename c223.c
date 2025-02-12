struct scm_fp_list *scm_fp_dup(struct scm_fp_list *fpl)
{
	struct scm_fp_list *new_fpl;
	int i;

	if (!fpl)
		return NULL;

	new_fpl = kmalloc(sizeof(*fpl), GFP_KERNEL);
	if (new_fpl) {
		for (i=fpl->count-1; i>=0; i--)
			get_file(fpl->fp[i]);
		memcpy(new_fpl, fpl, sizeof(*fpl));
	}
	return new_fpl;
}