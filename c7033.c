long strncpy_from_user(char *dst, const char __user *src, long count)
{
	unsigned long max_addr, src_addr;

	if (unlikely(count <= 0))
		return 0;

	max_addr = user_addr_max();
	src_addr = (unsigned long)src;
	if (likely(src_addr < max_addr)) {
		unsigned long max = max_addr - src_addr;
		long retval;

		kasan_check_write(dst, count);
		check_object_size(dst, count, false);
		user_access_begin();
		retval = do_strncpy_from_user(dst, src, count, max);
		user_access_end();
		return retval;
	}
	return -EFAULT;
}