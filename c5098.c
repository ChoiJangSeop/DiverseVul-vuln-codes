int devmem_is_allowed(unsigned long pagenr)
{
	if (pagenr < 256)
		return 1;
	if (iomem_is_exclusive(pagenr << PAGE_SHIFT))
		return 0;
	if (!page_is_ram(pagenr))
		return 1;
	return 0;
}