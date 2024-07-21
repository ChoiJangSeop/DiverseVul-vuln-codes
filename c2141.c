evbuffer_add_vprintf(struct evbuffer *buf, const char *fmt, va_list ap)
{
	char *buffer;
	size_t space;
	int sz, result = -1;
	va_list aq;
	struct evbuffer_chain *chain;


	EVBUFFER_LOCK(buf);

	if (buf->freeze_end) {
		goto done;
	}

	/* make sure that at least some space is available */
	if ((chain = evbuffer_expand_singlechain(buf, 64)) == NULL)
		goto done;

	for (;;) {
#if 0
		size_t used = chain->misalign + chain->off;
		buffer = (char *)chain->buffer + chain->misalign + chain->off;
		EVUTIL_ASSERT(chain->buffer_len >= used);
		space = chain->buffer_len - used;
#endif
		buffer = (char*) CHAIN_SPACE_PTR(chain);
		space = (size_t) CHAIN_SPACE_LEN(chain);

#ifndef va_copy
#define	va_copy(dst, src)	memcpy(&(dst), &(src), sizeof(va_list))
#endif
		va_copy(aq, ap);

		sz = evutil_vsnprintf(buffer, space, fmt, aq);

		va_end(aq);

		if (sz < 0)
			goto done;
		if ((size_t)sz < space) {
			chain->off += sz;
			buf->total_len += sz;
			buf->n_add_for_cb += sz;

			advance_last_with_data(buf);
			evbuffer_invoke_callbacks_(buf);
			result = sz;
			goto done;
		}
		if ((chain = evbuffer_expand_singlechain(buf, sz + 1)) == NULL)
			goto done;
	}
	/* NOTREACHED */

done:
	EVBUFFER_UNLOCK(buf);
	return result;
}