int wait_for_key_construction(struct key *key, bool intr)
{
	int ret;

	ret = wait_on_bit(&key->flags, KEY_FLAG_USER_CONSTRUCT,
			  intr ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);
	if (ret)
		return -ERESTARTSYS;
	if (test_bit(KEY_FLAG_NEGATIVE, &key->flags)) {
		smp_rmb();
		return key->reject_error;
	}
	return key_validate(key);
}