static int num_pages_spanned(struct iovec *iov)
{
	return
	((PAGE_ALIGN((unsigned long)iov->iov_base + iov->iov_len) -
	((unsigned long)iov->iov_base & PAGE_MASK)) >> PAGE_SHIFT);
}