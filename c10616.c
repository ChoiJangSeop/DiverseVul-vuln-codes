static int compat_copy_entry_to_user(struct ipt_entry *e,
		void __user **dstptr, compat_uint_t *size)
{
	struct ipt_entry_target __user *t;
	struct compat_ipt_entry __user *ce;
	u_int16_t target_offset, next_offset;
	compat_uint_t origsize;
	int ret;

	ret = -EFAULT;
	origsize = *size;
	ce = (struct compat_ipt_entry __user *)*dstptr;
	if (copy_to_user(ce, e, sizeof(struct ipt_entry)))
		goto out;

	*dstptr += sizeof(struct compat_ipt_entry);
	ret = IPT_MATCH_ITERATE(e, compat_copy_match_to_user, dstptr, size);
	target_offset = e->target_offset - (origsize - *size);
	if (ret)
		goto out;
	t = ipt_get_target(e);
	if (t->u.kernel.target->compat)
		ret = t->u.kernel.target->compat(t, dstptr, size,
				COMPAT_TO_USER);
	else
		ret = xt_compat_target(t, dstptr, size, COMPAT_TO_USER);
	if (ret)
		goto out;
	ret = -EFAULT;
	next_offset = e->next_offset - (origsize - *size);
	if (put_user(target_offset, &ce->target_offset))
		goto out;
	if (put_user(next_offset, &ce->next_offset))
		goto out;
	return 0;
out:
	return ret;
}