check_entry(const struct ipt_entry *e, const char *name)
{
	const struct xt_entry_target *t;

	if (!ip_checkentry(&e->ip)) {
		duprintf("ip check failed %p %s.\n", e, name);
		return -EINVAL;
	}

	if (e->target_offset + sizeof(struct xt_entry_target) >
	    e->next_offset)
		return -EINVAL;

	t = ipt_get_target_c(e);
	if (e->target_offset + t->u.target_size > e->next_offset)
		return -EINVAL;

	return 0;
}