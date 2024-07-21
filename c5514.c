struct page *dma_alloc_from_contiguous(struct device *dev, int count,
				       unsigned int align)
{
	if (align > CONFIG_CMA_ALIGNMENT)
		align = CONFIG_CMA_ALIGNMENT;

	return cma_alloc(dev_get_cma_area(dev), count, align);
}