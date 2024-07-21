long compat_put_bitmap(compat_ulong_t __user *umask, unsigned long *mask,
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
		unsigned long m = *mask++;
		unsafe_put_user((compat_ulong_t)m, umask++, Efault);
		unsafe_put_user(m >> BITS_PER_COMPAT_LONG, umask++, Efault);
		nr_compat_longs -= 2;
	}
	if (nr_compat_longs)
		unsafe_put_user((compat_ulong_t)*mask, umask++, Efault);
	user_access_end();
	return 0;
Efault:
	user_access_end();
	return -EFAULT;
}