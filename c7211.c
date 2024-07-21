dma_async_memcpy_buf_to_pg(struct dma_chan *chan, struct page *page,
			   unsigned int offset, void *kdata, size_t len)
{
	return dma_async_memcpy_pg_to_pg(chan, page, offset,
					 virt_to_page(kdata),
					 (unsigned long) kdata & ~PAGE_MASK, len);
}