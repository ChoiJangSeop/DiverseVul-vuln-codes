long compat_get_bitmap(unsigned long *mask, const compat_ulong_t __user *umask,
		       unsigned long bitmap_size)
{
	unsigned long nr_compat_longs;

	/* align bitmap up to nearest compat_long_t boundary */
	bitmap_size = ALIGN(bitmap_size, BITS_PER_COMPAT_LONG);
	nr_compat_longs = BITS_TO_COMPAT_LONGS(bitmap_size);

	if (!access_ok(umask, bitmap_size / 8))
		return -EFAULT;

	user_access_begin();
	while (nr_compat_longs > 1) {
		compat_ulong_t l1, l2;
		unsafe_get_user(l1, umask++, Efault);
		unsafe_get_user(l2, umask++, Efault);
		*mask++ = ((unsigned long)l2 << BITS_PER_COMPAT_LONG) | l1;
		nr_compat_longs -= 2;
	}
	if (nr_compat_longs)
		unsafe_get_user(*mask, umask++, Efault);
	user_access_end();
	return 0;

Efault:
	user_access_end();
	return -EFAULT;
}