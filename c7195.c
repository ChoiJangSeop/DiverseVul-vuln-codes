dma_async_memcpy_buf_to_buf(struct dma_chan *chan, void *dest,
			    void *src, size_t len)
{
	return dma_async_memcpy_pg_to_pg(chan, virt_to_page(dest),
					 (unsigned long) dest & ~PAGE_MASK,
					 virt_to_page(src),
					 (unsigned long) src & ~PAGE_MASK, len);
}