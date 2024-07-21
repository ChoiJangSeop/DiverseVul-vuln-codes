static void __unmap_hugepage_range(struct mmu_gather *tlb, struct vm_area_struct *vma,
				   unsigned long start, unsigned long end,
				   struct page *ref_page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long address;
	pte_t *ptep;
	pte_t pte;
	spinlock_t *ptl;
	struct page *page;
	struct hstate *h = hstate_vma(vma);
	unsigned long sz = huge_page_size(h);
	struct mmu_notifier_range range;

	WARN_ON(!is_vm_hugetlb_page(vma));
	BUG_ON(start & ~huge_page_mask(h));
	BUG_ON(end & ~huge_page_mask(h));

	/*
	 * This is a hugetlb vma, all the pte entries should point
	 * to huge page.
	 */
	tlb_change_page_size(tlb, sz);
	tlb_start_vma(tlb, vma);

	/*
	 * If sharing possible, alert mmu notifiers of worst case.
	 */
	mmu_notifier_range_init(&range, MMU_NOTIFY_UNMAP, 0, vma, mm, start,
				end);
	adjust_range_if_pmd_sharing_possible(vma, &range.start, &range.end);
	mmu_notifier_invalidate_range_start(&range);
	address = start;
	for (; address < end; address += sz) {
		ptep = huge_pte_offset(mm, address, sz);
		if (!ptep)
			continue;

		ptl = huge_pte_lock(h, mm, ptep);
		if (huge_pmd_unshare(mm, vma, &address, ptep)) {
			spin_unlock(ptl);
			/*
			 * We just unmapped a page of PMDs by clearing a PUD.
			 * The caller's TLB flush range should cover this area.
			 */
			continue;
		}

		pte = huge_ptep_get(ptep);
		if (huge_pte_none(pte)) {
			spin_unlock(ptl);
			continue;
		}

		/*
		 * Migrating hugepage or HWPoisoned hugepage is already
		 * unmapped and its refcount is dropped, so just clear pte here.
		 */
		if (unlikely(!pte_present(pte))) {
			huge_pte_clear(mm, address, ptep, sz);
			spin_unlock(ptl);
			continue;
		}

		page = pte_page(pte);
		/*
		 * If a reference page is supplied, it is because a specific
		 * page is being unmapped, not a range. Ensure the page we
		 * are about to unmap is the actual page of interest.
		 */
		if (ref_page) {
			if (page != ref_page) {
				spin_unlock(ptl);
				continue;
			}
			/*
			 * Mark the VMA as having unmapped its page so that
			 * future faults in this VMA will fail rather than
			 * looking like data was lost
			 */
			set_vma_resv_flags(vma, HPAGE_RESV_UNMAPPED);
		}

		pte = huge_ptep_get_and_clear(mm, address, ptep);
		tlb_remove_huge_tlb_entry(h, tlb, ptep, address);
		if (huge_pte_dirty(pte))
			set_page_dirty(page);

		hugetlb_count_sub(pages_per_huge_page(h), mm);
		page_remove_rmap(page, true);

		spin_unlock(ptl);
		tlb_remove_page_size(tlb, page, huge_page_size(h));
		/*
		 * Bail out after unmapping reference page if supplied
		 */
		if (ref_page)
			break;
	}
	mmu_notifier_invalidate_range_end(&range);
	tlb_end_vma(tlb, vma);
}