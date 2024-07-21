void *jas_malloc(size_t size)
{
#if defined(MEMALLOC_ALIGN2)
	void *ptr;
abort();
	if (posix_memalign(&ptr, MEMALLOC_ALIGNMENT, size)) {
		return 0;
	}
	return ptr;
#endif
	return malloc(size);
}