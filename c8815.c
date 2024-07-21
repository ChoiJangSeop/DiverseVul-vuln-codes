int pud_set_huge(pud_t *pudp, phys_addr_t phys, pgprot_t prot)
{
	pgprot_t sect_prot = __pgprot(PUD_TYPE_SECT |
					pgprot_val(mk_sect_prot(prot)));
	BUG_ON(phys & ~PUD_MASK);
	set_pud(pudp, pfn_pud(__phys_to_pfn(phys), sect_prot));
	return 1;
}