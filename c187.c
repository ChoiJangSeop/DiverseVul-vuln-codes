void *jas_calloc(size_t nmemb, size_t size)
{
	void *ptr;
	size_t n;
	n = nmemb * size;
	if (!(ptr = jas_malloc(n * sizeof(char)))) {
		return 0;
	}
	memset(ptr, 0, n);
	return ptr;
}