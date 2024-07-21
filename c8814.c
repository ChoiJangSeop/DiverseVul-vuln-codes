int pmd_set_huge(pmd_t *pmdp, phys_addr_t phys, pgprot_t prot)
{
	pgprot_t sect_prot = __pgprot(PMD_TYPE_SECT |
					pgprot_val(mk_sect_prot(prot)));
	BUG_ON(phys & ~PMD_MASK);
	set_pmd(pmdp, pfn_pmd(__phys_to_pfn(phys), sect_prot));
	return 1;
}