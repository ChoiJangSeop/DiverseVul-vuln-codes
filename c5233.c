unsigned long arch_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long task_size = TASK_SIZE;
	int do_color_align, last_mmap;
	struct vm_unmapped_area_info info;

	if (len > task_size)
		return -ENOMEM;

	do_color_align = 0;
	if (filp || (flags & MAP_SHARED))
		do_color_align = 1;
	last_mmap = GET_LAST_MMAP(filp);

	if (flags & MAP_FIXED) {
		if ((flags & MAP_SHARED) && last_mmap &&
		    (addr - shared_align_offset(last_mmap, pgoff))
				& (SHM_COLOUR - 1))
			return -EINVAL;
		goto found_addr;
	}

	if (addr) {
		if (do_color_align && last_mmap)
			addr = COLOR_ALIGN(addr, last_mmap, pgoff);
		else
			addr = PAGE_ALIGN(addr);

		vma = find_vma(mm, addr);
		if (task_size - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			goto found_addr;
	}

	info.flags = 0;
	info.length = len;
	info.low_limit = mm->mmap_legacy_base;
	info.high_limit = mmap_upper_limit();
	info.align_mask = last_mmap ? (PAGE_MASK & (SHM_COLOUR - 1)) : 0;
	info.align_offset = shared_align_offset(last_mmap, pgoff);
	addr = vm_unmapped_area(&info);

found_addr:
	if (do_color_align && !last_mmap && !(addr & ~PAGE_MASK))
		SET_LAST_MMAP(filp, addr - (pgoff << PAGE_SHIFT));

	return addr;
}