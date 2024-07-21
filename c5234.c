static int slice_area_is_free(struct mm_struct *mm, unsigned long addr,
			      unsigned long len)
{
	struct vm_area_struct *vma;

	if ((mm->task_size - len) < addr)
		return 0;
	vma = find_vma(mm, addr);
	return (!vma || (addr + len) <= vma->vm_start);
}