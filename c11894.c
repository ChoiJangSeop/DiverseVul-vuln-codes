tlb_update_vma_flags(struct mmu_gather *tlb, struct vm_area_struct *vma)
{
	/*
	 * flush_tlb_range() implementations that look at VM_HUGETLB (tile,
	 * mips-4k) flush only large pages.
	 *
	 * flush_tlb_range() implementations that flush I-TLB also flush D-TLB
	 * (tile, xtensa, arm), so it's ok to just add VM_EXEC to an existing
	 * range.
	 *
	 * We rely on tlb_end_vma() to issue a flush, such that when we reset
	 * these values the batch is empty.
	 */
	tlb->vma_huge = is_vm_hugetlb_page(vma);
	tlb->vma_exec = !!(vma->vm_flags & VM_EXEC);
}