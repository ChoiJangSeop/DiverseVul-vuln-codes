int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
	struct vm_area_struct *prev;

	address &= PAGE_MASK;
	prev = vma->vm_prev;
	if (prev && prev->vm_end == address) {
		if (!(prev->vm_flags & VM_GROWSDOWN))
			return -ENOMEM;
	}
	return expand_downwards(vma, address);
}