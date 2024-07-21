unsigned long arch_get_unmapped_area(struct file *filp, unsigned long addr, unsigned long len,
				     unsigned long pgoff, unsigned long flags)
{
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;

	if (len > TASK_SIZE)
		return -ENOMEM;

	/* handle MAP_FIXED */
	if (flags & MAP_FIXED)
		return addr;

	/* only honour a hint if we're not going to clobber something doing so */
	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(current->mm, addr);
		if (TASK_SIZE - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			goto success;
	}

	/* search between the bottom of user VM and the stack grow area */
	info.flags = 0;
	info.length = len;
	info.low_limit = PAGE_SIZE;
	info.high_limit = (current->mm->start_stack - 0x00200000);
	info.align_mask = 0;
	info.align_offset = 0;
	addr = vm_unmapped_area(&info);
	if (!(addr & ~PAGE_MASK))
		goto success;
	VM_BUG_ON(addr != -ENOMEM);

	/* search from just above the WorkRAM area to the top of memory */
	info.low_limit = PAGE_ALIGN(0x80000000);
	info.high_limit = TASK_SIZE;
	addr = vm_unmapped_area(&info);
	if (!(addr & ~PAGE_MASK))
		goto success;
	VM_BUG_ON(addr != -ENOMEM);

#if 0
	printk("[area] l=%lx (ENOMEM) f='%s'\n",
	       len, filp ? filp->f_path.dentry->d_name.name : "");
#endif
	return -ENOMEM;

 success:
#if 0
	printk("[area] l=%lx ad=%lx f='%s'\n",
	       len, addr, filp ? filp->f_path.dentry->d_name.name : "");
#endif
	return addr;
} /* end arch_get_unmapped_area() */