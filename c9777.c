reallocarray (void *ptr,
	      size_t nmemb,
	      size_t size)
{
	assert (nmemb > 0 && size > 0);
	if (SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc (ptr, nmemb * size);
}