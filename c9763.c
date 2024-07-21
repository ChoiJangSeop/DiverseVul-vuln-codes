int __do_munmap(struct mm_struct *mm, unsigned long start, size_t len,
		struct list_head *uf, bool downgrade)
{
	unsigned long end;
	struct vm_area_struct *vma, *prev, *last;

	if ((offset_in_page(start)) || start > TASK_SIZE || len > TASK_SIZE-start)
		return -EINVAL;

	len = PAGE_ALIGN(len);
	end = start + len;
	if (len == 0)
		return -EINVAL;

	/*
	 * arch_unmap() might do unmaps itself.  It must be called
	 * and finish any rbtree manipulation before this code
	 * runs and also starts to manipulate the rbtree.
	 */
	arch_unmap(mm, start, end);

	/* Find the first overlapping VMA */
	vma = find_vma(mm, start);
	if (!vma)
		return 0;
	prev = vma->vm_prev;
	/* we have  start < vma->vm_end  */

	/* if it doesn't overlap, we have nothing.. */
	if (vma->vm_start >= end)
		return 0;

	/*
	 * If we need to split any vma, do it now to save pain later.
	 *
	 * Note: mremap's move_vma VM_ACCOUNT handling assumes a partially
	 * unmapped vm_area_struct will remain in use: so lower split_vma
	 * places tmp vma above, and higher split_vma places tmp vma below.
	 */
	if (start > vma->vm_start) {
		int error;

		/*
		 * Make sure that map_count on return from munmap() will
		 * not exceed its limit; but let map_count go just above
		 * its limit temporarily, to help free resources as expected.
		 */
		if (end < vma->vm_end && mm->map_count >= sysctl_max_map_count)
			return -ENOMEM;

		error = __split_vma(mm, vma, start, 0);
		if (error)
			return error;
		prev = vma;
	}

	/* Does it split the last one? */
	last = find_vma(mm, end);
	if (last && end > last->vm_start) {
		int error = __split_vma(mm, last, end, 1);
		if (error)
			return error;
	}
	vma = prev ? prev->vm_next : mm->mmap;

	if (unlikely(uf)) {
		/*
		 * If userfaultfd_unmap_prep returns an error the vmas
		 * will remain splitted, but userland will get a
		 * highly unexpected error anyway. This is no
		 * different than the case where the first of the two
		 * __split_vma fails, but we don't undo the first
		 * split, despite we could. This is unlikely enough
		 * failure that it's not worth optimizing it for.
		 */
		int error = userfaultfd_unmap_prep(vma, start, end, uf);
		if (error)
			return error;
	}

	/*
	 * unlock any mlock()ed ranges before detaching vmas
	 */
	if (mm->locked_vm) {
		struct vm_area_struct *tmp = vma;
		while (tmp && tmp->vm_start < end) {
			if (tmp->vm_flags & VM_LOCKED) {
				mm->locked_vm -= vma_pages(tmp);
				munlock_vma_pages_all(tmp);
			}

			tmp = tmp->vm_next;
		}
	}

	/* Detach vmas from rbtree */
	detach_vmas_to_be_unmapped(mm, vma, prev, end);

	if (downgrade)
		mmap_write_downgrade(mm);

	unmap_region(mm, vma, prev, start, end);

	/* Fix up all other VM information */
	remove_vma_list(mm, vma);

	return downgrade ? 1 : 0;
}