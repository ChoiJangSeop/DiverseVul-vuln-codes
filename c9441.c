static void drop_futex_key_refs(union futex_key *key)
{
	if (!key->both.ptr) {
		/* If we're here then we tried to put a key we failed to get */
		WARN_ON_ONCE(1);
		return;
	}

	if (!IS_ENABLED(CONFIG_MMU))
		return;

	switch (key->both.offset & (FUT_OFF_INODE|FUT_OFF_MMSHARED)) {
	case FUT_OFF_INODE:
		iput(key->shared.inode);
		break;
	case FUT_OFF_MMSHARED:
		mmdrop(key->private.mm);
		break;
	}
}