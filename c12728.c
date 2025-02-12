int hugepage_madvise(struct vm_area_struct *vma,
		     unsigned long *vm_flags, int advice)
{
	switch (advice) {
	case MADV_HUGEPAGE:
		/*
		 * Be somewhat over-protective like KSM for now!
		 */
		if (*vm_flags & (VM_HUGEPAGE |
				 VM_SHARED   | VM_MAYSHARE   |
				 VM_PFNMAP   | VM_IO      | VM_DONTEXPAND |
				 VM_RESERVED | VM_HUGETLB | VM_INSERTPAGE |
				 VM_MIXEDMAP | VM_SAO))
			return -EINVAL;
		*vm_flags &= ~VM_NOHUGEPAGE;
		*vm_flags |= VM_HUGEPAGE;
		/*
		 * If the vma become good for khugepaged to scan,
		 * register it here without waiting a page fault that
		 * may not happen any time soon.
		 */
		if (unlikely(khugepaged_enter_vma_merge(vma)))
			return -ENOMEM;
		break;
	case MADV_NOHUGEPAGE:
		/*
		 * Be somewhat over-protective like KSM for now!
		 */
		if (*vm_flags & (VM_NOHUGEPAGE |
				 VM_SHARED   | VM_MAYSHARE   |
				 VM_PFNMAP   | VM_IO      | VM_DONTEXPAND |
				 VM_RESERVED | VM_HUGETLB | VM_INSERTPAGE |
				 VM_MIXEDMAP | VM_SAO))
			return -EINVAL;
		*vm_flags &= ~VM_HUGEPAGE;
		*vm_flags |= VM_NOHUGEPAGE;
		/*
		 * Setting VM_NOHUGEPAGE will prevent khugepaged from scanning
		 * this vma even if we leave the mm registered in khugepaged if
		 * it got registered before VM_NOHUGEPAGE was set.
		 */
		break;
	}

	return 0;
}