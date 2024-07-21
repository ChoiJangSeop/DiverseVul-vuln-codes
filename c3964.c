check_entry(const struct ip6t_entry *e, const char *name)
{
	const struct xt_entry_target *t;

	if (!ip6_checkentry(&e->ipv6)) {
		duprintf("ip_tables: ip check failed %p %s.\n", e, name);
		return -EINVAL;
	}

	if (e->target_offset + sizeof(struct xt_entry_target) >
	    e->next_offset)
		return -EINVAL;

	t = ip6t_get_target_c(e);
	if (e->target_offset + t->u.target_size > e->next_offset)
		return -EINVAL;

	return 0;
}