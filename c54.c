static int copy_from_user_mmap_sem(void *dst, const void __user *src, size_t n)
{
	int partial;

	pagefault_disable();
	partial = __copy_from_user_inatomic(dst, src, n);
	pagefault_enable();

	/*
	 * Didn't copy everything, drop the mmap_sem and do a faulting copy
	 */
	if (unlikely(partial)) {
		up_read(&current->mm->mmap_sem);
		partial = copy_from_user(dst, src, n);
		down_read(&current->mm->mmap_sem);
	}

	return partial;
}