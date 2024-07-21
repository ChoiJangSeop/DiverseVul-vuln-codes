compat_calc_match(struct ipt_entry_match *m, int * size)
{
	if (m->u.kernel.match->compat)
		m->u.kernel.match->compat(m, NULL, size, COMPAT_CALC_SIZE);
	else
		xt_compat_match(m, NULL, size, COMPAT_CALC_SIZE);
	return 0;
}