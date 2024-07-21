int move_hugetlb_page_tables(struct vm_area_struct *vma,
			     struct vm_area_struct *new_vma,
			     unsigned long old_addr, unsigned long new_addr,
			     unsigned long len)
{
	struct hstate *h = hstate_vma(vma);
	struct address_space *mapping = vma->vm_file->f_mapping;
	unsigned long sz = huge_page_size(h);
	struct mm_struct *mm = vma->vm_mm;
	unsigned long old_end = old_addr + len;
	unsigned long old_addr_copy;
	pte_t *src_pte, *dst_pte;
	struct mmu_notifier_range range;

	mmu_notifier_range_init(&range, MMU_NOTIFY_CLEAR, 0, vma, mm, old_addr,
				old_end);
	adjust_range_if_pmd_sharing_possible(vma, &range.start, &range.end);
	mmu_notifier_invalidate_range_start(&range);
	/* Prevent race with file truncation */
	i_mmap_lock_write(mapping);
	for (; old_addr < old_end; old_addr += sz, new_addr += sz) {
		src_pte = huge_pte_offset(mm, old_addr, sz);
		if (!src_pte)
			continue;
		if (huge_pte_none(huge_ptep_get(src_pte)))
			continue;

		/* old_addr arg to huge_pmd_unshare() is a pointer and so the
		 * arg may be modified. Pass a copy instead to preserve the
		 * value in old_addr.
		 */
		old_addr_copy = old_addr;

		if (huge_pmd_unshare(mm, vma, &old_addr_copy, src_pte))
			continue;

		dst_pte = huge_pte_alloc(mm, new_vma, new_addr, sz);
		if (!dst_pte)
			break;

		move_huge_pte(vma, old_addr, new_addr, src_pte);
	}
	i_mmap_unlock_write(mapping);
	flush_tlb_range(vma, old_end - len, old_end);
	mmu_notifier_invalidate_range_end(&range);

	return len + old_addr - old_end;
}