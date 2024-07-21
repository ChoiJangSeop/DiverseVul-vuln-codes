int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
	struct vm_area_struct *next;

	address &= PAGE_MASK;
	next = vma->vm_next;
	if (next && next->vm_start == address + PAGE_SIZE) {
		if (!(next->vm_flags & VM_GROWSUP))
			return -ENOMEM;
	}
	return expand_upwards(vma, address);
}