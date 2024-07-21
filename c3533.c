hugetlb_vmdelete_list(struct rb_root *root, pgoff_t start, pgoff_t end)
{
	struct vm_area_struct *vma;

	/*
	 * end == 0 indicates that the entire range after
	 * start should be unmapped.
	 */
	vma_interval_tree_foreach(vma, root, start, end ? end : ULONG_MAX) {
		unsigned long v_offset;

		/*
		 * Can the expression below overflow on 32-bit arches?
		 * No, because the interval tree returns us only those vmas
		 * which overlap the truncated area starting at pgoff,
		 * and no vma on a 32-bit arch can span beyond the 4GB.
		 */
		if (vma->vm_pgoff < start)
			v_offset = (start - vma->vm_pgoff) << PAGE_SHIFT;
		else
			v_offset = 0;

		if (end) {
			end = ((end - start) << PAGE_SHIFT) +
			       vma->vm_start + v_offset;
			if (end > vma->vm_end)
				end = vma->vm_end;
		} else
			end = vma->vm_end;

		unmap_hugepage_range(vma, vma->vm_start + v_offset, end, NULL);
	}
}