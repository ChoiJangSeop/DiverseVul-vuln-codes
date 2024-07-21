find_check_entry(struct arpt_entry *e, const char *name, unsigned int size)
{
	struct xt_entry_target *t;
	struct xt_target *target;
	int ret;

	ret = check_entry(e, name);
	if (ret)
		return ret;

	e->counters.pcnt = xt_percpu_counter_alloc();
	if (IS_ERR_VALUE(e->counters.pcnt))
		return -ENOMEM;

	t = arpt_get_target(e);
	target = xt_request_find_target(NFPROTO_ARP, t->u.user.name,
					t->u.user.revision);
	if (IS_ERR(target)) {
		duprintf("find_check_entry: `%s' not found\n", t->u.user.name);
		ret = PTR_ERR(target);
		goto out;
	}
	t->u.kernel.target = target;

	ret = check_target(e, name);
	if (ret)
		goto err;
	return 0;
err:
	module_put(t->u.kernel.target->me);
out:
	xt_percpu_counter_free(e->counters.pcnt);

	return ret;
}