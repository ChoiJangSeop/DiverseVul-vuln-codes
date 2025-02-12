static void mincore_pmd_range(struct vm_area_struct *vma, pud_t *pud,
			unsigned long addr, unsigned long end,
			unsigned char *vec)
{
	unsigned long next;
	pmd_t *pmd;

	pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_trans_huge(*pmd)) {
			if (mincore_huge_pmd(vma, pmd, addr, next, vec)) {
				vec += (next - addr) >> PAGE_SHIFT;
				continue;
			}
			/* fall through */
		}
		if (pmd_none_or_clear_bad(pmd))
			mincore_unmapped_range(vma, addr, next, vec);
		else
			mincore_pte_range(vma, pmd, addr, next, vec);
		vec += (next - addr) >> PAGE_SHIFT;
	} while (pmd++, addr = next, addr != end);
}