int xt_compat_check_entry_offsets(const void *base,
				  unsigned int target_offset,
				  unsigned int next_offset)
{
	const struct compat_xt_entry_target *t;
	const char *e = base;

	if (target_offset + sizeof(*t) > next_offset)
		return -EINVAL;

	t = (void *)(e + target_offset);
	if (t->u.target_size < sizeof(*t))
		return -EINVAL;

	if (target_offset + t->u.target_size > next_offset)
		return -EINVAL;

	if (strcmp(t->u.user.name, XT_STANDARD_TARGET) == 0 &&
	    target_offset + sizeof(struct compat_xt_standard_target) != next_offset)
		return -EINVAL;

	return 0;
}