phys_addr_t swiotlb_tbl_map_single(struct device *dev, phys_addr_t orig_addr,
		size_t mapping_size, size_t alloc_size,
		unsigned int alloc_align_mask, enum dma_data_direction dir,
		unsigned long attrs)
{
	struct io_tlb_mem *mem = dev->dma_io_tlb_mem;
	unsigned int offset = swiotlb_align_offset(dev, orig_addr);
	unsigned int i;
	int index;
	phys_addr_t tlb_addr;

	if (!mem)
		panic("Can not allocate SWIOTLB buffer earlier and can't now provide you with the DMA bounce buffer");

	if (cc_platform_has(CC_ATTR_MEM_ENCRYPT))
		pr_warn_once("Memory encryption is active and system is using DMA bounce buffers\n");

	if (mapping_size > alloc_size) {
		dev_warn_once(dev, "Invalid sizes (mapping: %zd bytes, alloc: %zd bytes)",
			      mapping_size, alloc_size);
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	index = swiotlb_find_slots(dev, orig_addr,
				   alloc_size + offset, alloc_align_mask);
	if (index == -1) {
		if (!(attrs & DMA_ATTR_NO_WARN))
			dev_warn_ratelimited(dev,
	"swiotlb buffer is full (sz: %zd bytes), total %lu (slots), used %lu (slots)\n",
				 alloc_size, mem->nslabs, mem->used);
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	/*
	 * Save away the mapping from the original address to the DMA address.
	 * This is needed when we sync the memory.  Then we sync the buffer if
	 * needed.
	 */
	for (i = 0; i < nr_slots(alloc_size + offset); i++)
		mem->slots[index + i].orig_addr = slot_addr(orig_addr, i);
	tlb_addr = slot_addr(mem->start, index) + offset;
	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
	    (!(attrs & DMA_ATTR_OVERWRITE) || dir == DMA_TO_DEVICE ||
	    dir == DMA_BIDIRECTIONAL))
		swiotlb_bounce(dev, tlb_addr, mapping_size, DMA_TO_DEVICE);
	return tlb_addr;
}