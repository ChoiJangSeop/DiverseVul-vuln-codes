long strnlen_user(const char __user *str, long count)
{
	unsigned long max_addr, src_addr;

	if (unlikely(count <= 0))
		return 0;

	max_addr = user_addr_max();
	src_addr = (unsigned long)str;
	if (likely(src_addr < max_addr)) {
		unsigned long max = max_addr - src_addr;
		long retval;

		user_access_begin();
		retval = do_strnlen_user(str, count, max);
		user_access_end();
		return retval;
	}
	return 0;
}