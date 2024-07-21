find_check_entry(struct ip6t_entry *e, struct net *net, const char *name,
		 unsigned int size)
{
	struct xt_entry_target *t;
	struct xt_target *target;
	int ret;
	unsigned int j;
	struct xt_mtchk_param mtpar;
	struct xt_entry_match *ematch;

	ret = check_entry(e, name);
	if (ret)
		return ret;

	e->counters.pcnt = xt_percpu_counter_alloc();
	if (IS_ERR_VALUE(e->counters.pcnt))
		return -ENOMEM;

	j = 0;
	mtpar.net	= net;
	mtpar.table     = name;
	mtpar.entryinfo = &e->ipv6;
	mtpar.hook_mask = e->comefrom;
	mtpar.family    = NFPROTO_IPV6;
	xt_ematch_foreach(ematch, e) {
		ret = find_check_match(ematch, &mtpar);
		if (ret != 0)
			goto cleanup_matches;
		++j;
	}

	t = ip6t_get_target(e);
	target = xt_request_find_target(NFPROTO_IPV6, t->u.user.name,
					t->u.user.revision);
	if (IS_ERR(target)) {
		duprintf("find_check_entry: `%s' not found\n", t->u.user.name);
		ret = PTR_ERR(target);
		goto cleanup_matches;
	}
	t->u.kernel.target = target;

	ret = check_target(e, net, name);
	if (ret)
		goto err;
	return 0;
 err:
	module_put(t->u.kernel.target->me);
 cleanup_matches:
	xt_ematch_foreach(ematch, e) {
		if (j-- == 0)
			break;
		cleanup_match(ematch, net);
	}

	xt_percpu_counter_free(e->counters.pcnt);

	return ret;
}