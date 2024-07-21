evbuffer_chain_new(size_t size)
{
	struct evbuffer_chain *chain;
	size_t to_alloc;

	size += EVBUFFER_CHAIN_SIZE;

	/* get the next largest memory that can hold the buffer */
	to_alloc = MIN_BUFFER_SIZE;
	while (to_alloc < size)
		to_alloc <<= 1;

	/* we get everything in one chunk */
	if ((chain = mm_malloc(to_alloc)) == NULL)
		return (NULL);

	memset(chain, 0, EVBUFFER_CHAIN_SIZE);

	chain->buffer_len = to_alloc - EVBUFFER_CHAIN_SIZE;

	/* this way we can manipulate the buffer to different addresses,
	 * which is required for mmap for example.
	 */
	chain->buffer = EVBUFFER_CHAIN_EXTRA(u_char, chain);

	chain->refcnt = 1;

	return (chain);
}