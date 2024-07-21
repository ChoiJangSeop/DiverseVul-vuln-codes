void arch_pick_mmap_layout(struct mm_struct *mm)
{
	unsigned long random_factor = 0UL;
	unsigned long gap;

	if (current->flags & PF_RANDOMIZE) {
		random_factor = get_random_int();
		if (test_thread_flag(TIF_32BIT))
			random_factor &= ((1 * 1024 * 1024) - 1);
		else
			random_factor = ((random_factor << PAGE_SHIFT) &
					 0xffffffffUL);
	}

	/*
	 * Fall back to the standard layout if the personality
	 * bit is set, or if the expected stack growth is unlimited:
	 */
	gap = rlimit(RLIMIT_STACK);
	if (!test_thread_flag(TIF_32BIT) ||
	    (current->personality & ADDR_COMPAT_LAYOUT) ||
	    gap == RLIM_INFINITY ||
	    sysctl_legacy_va_layout) {
		mm->mmap_base = TASK_UNMAPPED_BASE + random_factor;
		mm->get_unmapped_area = arch_get_unmapped_area;
		mm->unmap_area = arch_unmap_area;
	} else {
		/* We know it's 32-bit */
		unsigned long task_size = STACK_TOP32;

		if (gap < 128 * 1024 * 1024)
			gap = 128 * 1024 * 1024;
		if (gap > (task_size / 6 * 5))
			gap = (task_size / 6 * 5);

		mm->mmap_base = PAGE_ALIGN(task_size - gap - random_factor);
		mm->get_unmapped_area = arch_get_unmapped_area_topdown;
		mm->unmap_area = arch_unmap_area_topdown;
	}
}