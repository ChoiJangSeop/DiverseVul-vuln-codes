compat_check_calc_match(struct ipt_entry_match *m,
	    const char *name,
	    const struct ipt_ip *ip,
	    unsigned int hookmask,
	    int *size, int *i)
{
	struct ipt_match *match;

	match = try_then_request_module(xt_find_match(AF_INET, m->u.user.name,
						   m->u.user.revision),
					"ipt_%s", m->u.user.name);
	if (IS_ERR(match) || !match) {
		duprintf("compat_check_calc_match: `%s' not found\n",
				m->u.user.name);
		return match ? PTR_ERR(match) : -ENOENT;
	}
	m->u.kernel.match = match;

	if (m->u.kernel.match->compat)
		m->u.kernel.match->compat(m, NULL, size, COMPAT_CALC_SIZE);
	else
		xt_compat_match(m, NULL, size, COMPAT_CALC_SIZE);

	(*i)++;
	return 0;
}