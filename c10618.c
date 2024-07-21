check_compat_entry_size_and_hooks(struct ipt_entry *e,
			   struct xt_table_info *newinfo,
			   unsigned int *size,
			   unsigned char *base,
			   unsigned char *limit,
			   unsigned int *hook_entries,
			   unsigned int *underflows,
			   unsigned int *i,
			   const char *name)
{
	struct ipt_entry_target *t;
	struct ipt_target *target;
	u_int16_t entry_offset;
	int ret, off, h, j;

	duprintf("check_compat_entry_size_and_hooks %p\n", e);
	if ((unsigned long)e % __alignof__(struct compat_ipt_entry) != 0
	    || (unsigned char *)e + sizeof(struct compat_ipt_entry) >= limit) {
		duprintf("Bad offset %p, limit = %p\n", e, limit);
		return -EINVAL;
	}

	if (e->next_offset < sizeof(struct compat_ipt_entry) +
			sizeof(struct compat_xt_entry_target)) {
		duprintf("checking: element %p size %u\n",
			 e, e->next_offset);
		return -EINVAL;
	}

	if (!ip_checkentry(&e->ip)) {
		duprintf("ip_tables: ip check failed %p %s.\n", e, name);
		return -EINVAL;
	}

	off = 0;
	entry_offset = (void *)e - (void *)base;
	j = 0;
	ret = IPT_MATCH_ITERATE(e, compat_check_calc_match, name, &e->ip,
			e->comefrom, &off, &j);
	if (ret != 0)
		goto cleanup_matches;

	t = ipt_get_target(e);
	target = try_then_request_module(xt_find_target(AF_INET,
						     t->u.user.name,
						     t->u.user.revision),
					 "ipt_%s", t->u.user.name);
	if (IS_ERR(target) || !target) {
		duprintf("check_entry: `%s' not found\n", t->u.user.name);
		ret = target ? PTR_ERR(target) : -ENOENT;
		goto cleanup_matches;
	}
	t->u.kernel.target = target;

	if (t->u.kernel.target->compat)
		t->u.kernel.target->compat(t, NULL, &off, COMPAT_CALC_SIZE);
	else
		xt_compat_target(t, NULL, &off, COMPAT_CALC_SIZE);
	*size += off;
	ret = compat_add_offset(entry_offset, off);
	if (ret)
		goto out;

	/* Check hooks & underflows */
	for (h = 0; h < NF_IP_NUMHOOKS; h++) {
		if ((unsigned char *)e - base == hook_entries[h])
			newinfo->hook_entry[h] = hook_entries[h];
		if ((unsigned char *)e - base == underflows[h])
			newinfo->underflow[h] = underflows[h];
	}

	/* Clear counters and comefrom */
	e->counters = ((struct ipt_counters) { 0, 0 });
	e->comefrom = 0;

	(*i)++;
	return 0;

out:
	module_put(t->u.kernel.target->me);
cleanup_matches:
	IPT_MATCH_ITERATE(e, cleanup_match, &j);
	return ret;
}