dma_cookie_t dma_memcpy_to_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, unsigned char *kdata, size_t len)
{
	int iov_byte_offset;
	int copy;
	dma_cookie_t dma_cookie = 0;
	int iovec_idx;
	int page_idx;

	if (!chan)
		return memcpy_toiovec(iov, kdata, len);

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		/* skip already used-up iovecs */
		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		/* break up copies to not cross page boundary */
		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			dma_cookie = dma_async_memcpy_buf_to_pg(chan,
					page_list->pages[page_idx],
					iov_byte_offset,
					kdata,
					copy);
			/* poll for a descriptor slot */
			if (unlikely(dma_cookie < 0)) {
				dma_async_issue_pending(chan);
				continue;
			}

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			if (!len)
				return dma_cookie;

			kdata += copy;
			iov_byte_offset = 0;
			page_idx++;
		}
		iovec_idx++;
	}

	/* really bad if we ever run out of iovecs */
	BUG();
	return -EFAULT;
}