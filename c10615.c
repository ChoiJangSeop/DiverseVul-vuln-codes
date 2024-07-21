static int compat_copy_entry_from_user(struct ipt_entry *e, void **dstptr,
	unsigned int *size, const char *name,
	struct xt_table_info *newinfo, unsigned char *base)
{
	struct ipt_entry_target *t;
	struct ipt_target *target;
	struct ipt_entry *de;
	unsigned int origsize;
	int ret, h, j;

	ret = 0;
	origsize = *size;
	de = (struct ipt_entry *)*dstptr;
	memcpy(de, e, sizeof(struct ipt_entry));

	j = 0;
	*dstptr += sizeof(struct compat_ipt_entry);
	ret = IPT_MATCH_ITERATE(e, compat_copy_match_from_user, dstptr, size,
			name, &de->ip, de->comefrom, &j);
	if (ret)
		goto cleanup_matches;
	de->target_offset = e->target_offset - (origsize - *size);
	t = ipt_get_target(e);
	target = t->u.kernel.target;
	if (target->compat)
		target->compat(t, dstptr, size, COMPAT_FROM_USER);
	else
		xt_compat_target(t, dstptr, size, COMPAT_FROM_USER);

	de->next_offset = e->next_offset - (origsize - *size);
	for (h = 0; h < NF_IP_NUMHOOKS; h++) {
		if ((unsigned char *)de - base < newinfo->hook_entry[h])
			newinfo->hook_entry[h] -= origsize - *size;
		if ((unsigned char *)de - base < newinfo->underflow[h])
			newinfo->underflow[h] -= origsize - *size;
	}

	t = ipt_get_target(de);
	target = t->u.kernel.target;
	ret = xt_check_target(target, AF_INET, t->u.target_size - sizeof(*t),
			      name, e->comefrom, e->ip.proto,
			      e->ip.invflags & IPT_INV_PROTO);
	if (ret)
		goto err;

	ret = -EINVAL;
	if (t->u.kernel.target == &ipt_standard_target) {
		if (!standard_check(t, *size))
			goto err;
	} else if (t->u.kernel.target->checkentry
		   && !t->u.kernel.target->checkentry(name, de, target,
						      t->data, de->comefrom)) {
		duprintf("ip_tables: compat: check failed for `%s'.\n",
			 t->u.kernel.target->name);
		goto err;
	}
	ret = 0;
	return ret;

err:
	module_put(t->u.kernel.target->me);
cleanup_matches:
	IPT_MATCH_ITERATE(e, cleanup_match, &j);
	return ret;
}