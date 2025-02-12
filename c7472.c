static int userfaultfd_unregister(struct userfaultfd_ctx *ctx,
				  unsigned long arg)
{
	struct mm_struct *mm = ctx->mm;
	struct vm_area_struct *vma, *prev, *cur;
	int ret;
	struct uffdio_range uffdio_unregister;
	unsigned long new_flags;
	bool found;
	unsigned long start, end, vma_end;
	const void __user *buf = (void __user *)arg;

	ret = -EFAULT;
	if (copy_from_user(&uffdio_unregister, buf, sizeof(uffdio_unregister)))
		goto out;

	ret = validate_range(mm, uffdio_unregister.start,
			     uffdio_unregister.len);
	if (ret)
		goto out;

	start = uffdio_unregister.start;
	end = start + uffdio_unregister.len;

	ret = -ENOMEM;
	if (!mmget_not_zero(mm))
		goto out;

	down_write(&mm->mmap_sem);
	vma = find_vma_prev(mm, start, &prev);
	if (!vma)
		goto out_unlock;

	/* check that there's at least one vma in the range */
	ret = -EINVAL;
	if (vma->vm_start >= end)
		goto out_unlock;

	/*
	 * If the first vma contains huge pages, make sure start address
	 * is aligned to huge page size.
	 */
	if (is_vm_hugetlb_page(vma)) {
		unsigned long vma_hpagesize = vma_kernel_pagesize(vma);

		if (start & (vma_hpagesize - 1))
			goto out_unlock;
	}

	/*
	 * Search for not compatible vmas.
	 */
	found = false;
	ret = -EINVAL;
	for (cur = vma; cur && cur->vm_start < end; cur = cur->vm_next) {
		cond_resched();

		BUG_ON(!!cur->vm_userfaultfd_ctx.ctx ^
		       !!(cur->vm_flags & (VM_UFFD_MISSING | VM_UFFD_WP)));

		/*
		 * Check not compatible vmas, not strictly required
		 * here as not compatible vmas cannot have an
		 * userfaultfd_ctx registered on them, but this
		 * provides for more strict behavior to notice
		 * unregistration errors.
		 */
		if (!vma_can_userfault(cur))
			goto out_unlock;

		found = true;
	}
	BUG_ON(!found);

	if (vma->vm_start < start)
		prev = vma;

	ret = 0;
	do {
		cond_resched();

		BUG_ON(!vma_can_userfault(vma));

		/*
		 * Nothing to do: this vma is already registered into this
		 * userfaultfd and with the right tracking mode too.
		 */
		if (!vma->vm_userfaultfd_ctx.ctx)
			goto skip;

		WARN_ON(!(vma->vm_flags & VM_MAYWRITE));

		if (vma->vm_start > start)
			start = vma->vm_start;
		vma_end = min(end, vma->vm_end);

		if (userfaultfd_missing(vma)) {
			/*
			 * Wake any concurrent pending userfault while
			 * we unregister, so they will not hang
			 * permanently and it avoids userland to call
			 * UFFDIO_WAKE explicitly.
			 */
			struct userfaultfd_wake_range range;
			range.start = start;
			range.len = vma_end - start;
			wake_userfault(vma->vm_userfaultfd_ctx.ctx, &range);
		}

		new_flags = vma->vm_flags & ~(VM_UFFD_MISSING | VM_UFFD_WP);
		prev = vma_merge(mm, prev, start, vma_end, new_flags,
				 vma->anon_vma, vma->vm_file, vma->vm_pgoff,
				 vma_policy(vma),
				 NULL_VM_UFFD_CTX);
		if (prev) {
			vma = prev;
			goto next;
		}
		if (vma->vm_start < start) {
			ret = split_vma(mm, vma, start, 1);
			if (ret)
				break;
		}
		if (vma->vm_end > end) {
			ret = split_vma(mm, vma, end, 0);
			if (ret)
				break;
		}
	next:
		/*
		 * In the vma_merge() successful mprotect-like case 8:
		 * the next vma was merged into the current one and
		 * the current one has not been updated yet.
		 */
		vma->vm_flags = new_flags;
		vma->vm_userfaultfd_ctx = NULL_VM_UFFD_CTX;

	skip:
		prev = vma;
		start = vma->vm_end;
		vma = vma->vm_next;
	} while (vma && vma->vm_start < end);
out_unlock:
	up_write(&mm->mmap_sem);
	mmput(mm);
out:
	return ret;
}