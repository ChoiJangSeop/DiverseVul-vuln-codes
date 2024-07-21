hugetlb_vmtruncate_list(struct rb_root *root, pgoff_t pgoff)
{
	struct vm_area_struct *vma;

	vma_interval_tree_foreach(vma, root, pgoff, ULONG_MAX) {
		unsigned long v_offset;

		/*
		 * Can the expression below overflow on 32-bit arches?
		 * No, because the interval tree returns us only those vmas
		 * which overlap the truncated area starting at pgoff,
		 * and no vma on a 32-bit arch can span beyond the 4GB.
		 */
		if (vma->vm_pgoff < pgoff)
			v_offset = (pgoff - vma->vm_pgoff) << PAGE_SHIFT;
		else
			v_offset = 0;

		unmap_hugepage_range(vma, vma->vm_start + v_offset,
				     vma->vm_end, NULL);
	}
}