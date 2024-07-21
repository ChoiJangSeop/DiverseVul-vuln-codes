radix__arch_get_unmapped_area_topdown(struct file *filp,
				     const unsigned long addr0,
				     const unsigned long len,
				     const unsigned long pgoff,
				     const unsigned long flags)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	unsigned long addr = addr0;
	struct vm_unmapped_area_info info;

	if (unlikely(addr > mm->context.addr_limit &&
		     mm->context.addr_limit != TASK_SIZE))
		mm->context.addr_limit = TASK_SIZE;

	/* requested length too big for entire address space */
	if (len > mm->task_size - mmap_min_addr)
		return -ENOMEM;

	if (flags & MAP_FIXED)
		return addr;

	/* requesting a specific address */
	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (mm->task_size - len >= addr && addr >= mmap_min_addr &&
				(!vma || addr + len <= vma->vm_start))
			return addr;
	}

	info.flags = VM_UNMAPPED_AREA_TOPDOWN;
	info.length = len;
	info.low_limit = max(PAGE_SIZE, mmap_min_addr);
	info.high_limit = mm->mmap_base;
	info.align_mask = 0;

	if (addr > DEFAULT_MAP_WINDOW)
		info.high_limit += mm->context.addr_limit - DEFAULT_MAP_WINDOW;

	addr = vm_unmapped_area(&info);
	if (!(addr & ~PAGE_MASK))
		return addr;
	VM_BUG_ON(addr != -ENOMEM);

	/*
	 * A failed mmap() very likely causes application failure,
	 * so fall back to the bottom-up function here. This scenario
	 * can happen with large stack limits and large mmap()
	 * allocations.
	 */
	return radix__arch_get_unmapped_area(filp, addr0, len, pgoff, flags);
}