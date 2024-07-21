radix__arch_get_unmapped_area(struct file *filp, unsigned long addr,
			     unsigned long len, unsigned long pgoff,
			     unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;

	if (unlikely(addr > mm->context.addr_limit &&
		     mm->context.addr_limit != TASK_SIZE))
		mm->context.addr_limit = TASK_SIZE;

	if (len > mm->task_size - mmap_min_addr)
		return -ENOMEM;

	if (flags & MAP_FIXED)
		return addr;

	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (mm->task_size - len >= addr && addr >= mmap_min_addr &&
		    (!vma || addr + len <= vma->vm_start))
			return addr;
	}

	info.flags = 0;
	info.length = len;
	info.low_limit = mm->mmap_base;
	info.align_mask = 0;

	if (unlikely(addr > DEFAULT_MAP_WINDOW))
		info.high_limit = mm->context.addr_limit;
	else
		info.high_limit = DEFAULT_MAP_WINDOW;

	return vm_unmapped_area(&info);
}