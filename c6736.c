static void move_ptes(struct vm_area_struct *vma, pmd_t *old_pmd,
		unsigned long old_addr, unsigned long old_end,
		struct vm_area_struct *new_vma, pmd_t *new_pmd,
		unsigned long new_addr, bool need_rmap_locks, bool *need_flush)
{
	struct mm_struct *mm = vma->vm_mm;
	pte_t *old_pte, *new_pte, pte;
	spinlock_t *old_ptl, *new_ptl;
	bool force_flush = false;
	unsigned long len = old_end - old_addr;

	/*
	 * When need_rmap_locks is true, we take the i_mmap_rwsem and anon_vma
	 * locks to ensure that rmap will always observe either the old or the
	 * new ptes. This is the easiest way to avoid races with
	 * truncate_pagecache(), page migration, etc...
	 *
	 * When need_rmap_locks is false, we use other ways to avoid
	 * such races:
	 *
	 * - During exec() shift_arg_pages(), we use a specially tagged vma
	 *   which rmap call sites look for using is_vma_temporary_stack().
	 *
	 * - During mremap(), new_vma is often known to be placed after vma
	 *   in rmap traversal order. This ensures rmap will always observe
	 *   either the old pte, or the new pte, or both (the page table locks
	 *   serialize access to individual ptes, but only rmap traversal
	 *   order guarantees that we won't miss both the old and new ptes).
	 */
	if (need_rmap_locks)
		take_rmap_locks(vma);

	/*
	 * We don't have to worry about the ordering of src and dst
	 * pte locks because exclusive mmap_sem prevents deadlock.
	 */
	old_pte = pte_offset_map_lock(mm, old_pmd, old_addr, &old_ptl);
	new_pte = pte_offset_map(new_pmd, new_addr);
	new_ptl = pte_lockptr(mm, new_pmd);
	if (new_ptl != old_ptl)
		spin_lock_nested(new_ptl, SINGLE_DEPTH_NESTING);
	flush_tlb_batched_pending(vma->vm_mm);
	arch_enter_lazy_mmu_mode();

	for (; old_addr < old_end; old_pte++, old_addr += PAGE_SIZE,
				   new_pte++, new_addr += PAGE_SIZE) {
		if (pte_none(*old_pte))
			continue;

		pte = ptep_get_and_clear(mm, old_addr, old_pte);
		/*
		 * If we are remapping a dirty PTE, make sure
		 * to flush TLB before we drop the PTL for the
		 * old PTE or we may race with page_mkclean().
		 *
		 * This check has to be done after we removed the
		 * old PTE from page tables or another thread may
		 * dirty it after the check and before the removal.
		 */
		if (pte_present(pte) && pte_dirty(pte))
			force_flush = true;
		pte = move_pte(pte, new_vma->vm_page_prot, old_addr, new_addr);
		pte = move_soft_dirty_pte(pte);
		set_pte_at(mm, new_addr, new_pte, pte);
	}

	arch_leave_lazy_mmu_mode();
	if (new_ptl != old_ptl)
		spin_unlock(new_ptl);
	pte_unmap(new_pte - 1);
	if (force_flush)
		flush_tlb_range(vma, old_end - len, old_end);
	else
		*need_flush = true;
	pte_unmap_unlock(old_pte - 1, old_ptl);
	if (need_rmap_locks)
		drop_rmap_locks(vma);
}