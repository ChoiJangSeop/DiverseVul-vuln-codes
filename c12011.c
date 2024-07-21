SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
{
	profile_munmap(addr);
	return __vm_munmap(addr, len, true);
}