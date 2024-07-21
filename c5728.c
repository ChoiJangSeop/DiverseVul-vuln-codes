static void touch_pmd(struct vm_area_struct *vma, unsigned long addr,
		pmd_t *pmd)
{
	pmd_t _pmd;

	/*
	 * We should set the dirty bit only for FOLL_WRITE but for now
	 * the dirty bit in the pmd is meaningless.  And if the dirty
	 * bit will become meaningful and we'll only set it with
	 * FOLL_WRITE, an atomic set_bit will be required on the pmd to
	 * set the young bit, instead of the current set_pmd_at.
	 */
	_pmd = pmd_mkyoung(pmd_mkdirty(*pmd));
	if (pmdp_set_access_flags(vma, addr & HPAGE_PMD_MASK,
				pmd, _pmd,  1))
		update_mmu_cache_pmd(vma, addr, pmd);
}