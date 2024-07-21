static void touch_pud(struct vm_area_struct *vma, unsigned long addr,
		pud_t *pud)
{
	pud_t _pud;

	/*
	 * We should set the dirty bit only for FOLL_WRITE but for now
	 * the dirty bit in the pud is meaningless.  And if the dirty
	 * bit will become meaningful and we'll only set it with
	 * FOLL_WRITE, an atomic set_bit will be required on the pud to
	 * set the young bit, instead of the current set_pud_at.
	 */
	_pud = pud_mkyoung(pud_mkdirty(*pud));
	if (pudp_set_access_flags(vma, addr & HPAGE_PUD_MASK,
				pud, _pud,  1))
		update_mmu_cache_pud(vma, addr, pud);
}