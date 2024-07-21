static int compat_calc_entry(struct ipt_entry *e, struct xt_table_info *info,
		void *base, struct xt_table_info *newinfo)
{
	struct ipt_entry_target *t;
	u_int16_t entry_offset;
	int off, i, ret;

	off = 0;
	entry_offset = (void *)e - base;
	IPT_MATCH_ITERATE(e, compat_calc_match, &off);
	t = ipt_get_target(e);
	if (t->u.kernel.target->compat)
		t->u.kernel.target->compat(t, NULL, &off, COMPAT_CALC_SIZE);
	else
		xt_compat_target(t, NULL, &off, COMPAT_CALC_SIZE);
	newinfo->size -= off;
	ret = compat_add_offset(entry_offset, off);
	if (ret)
		return ret;

	for (i = 0; i< NF_IP_NUMHOOKS; i++) {
		if (info->hook_entry[i] && (e < (struct ipt_entry *)
				(base + info->hook_entry[i])))
			newinfo->hook_entry[i] -= off;
		if (info->underflow[i] && (e < (struct ipt_entry *)
				(base + info->underflow[i])))
			newinfo->underflow[i] -= off;
	}
	return 0;
}