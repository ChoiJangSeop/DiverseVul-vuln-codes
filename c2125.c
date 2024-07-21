evbuffer_add_vprintf(struct evbuffer *buf, const char *fmt, va_list ap)
{
	char *buffer;
	size_t space;
	size_t oldoff = buf->off;
	int sz;
	va_list aq;

	/* make sure that at least some space is available */
	evbuffer_expand(buf, 64);
	for (;;) {
		size_t used = buf->misalign + buf->off;
		buffer = (char *)buf->buffer + buf->off;
		assert(buf->totallen >= used);
		space = buf->totallen - used;

#ifndef va_copy
#define	va_copy(dst, src)	memcpy(&(dst), &(src), sizeof(va_list))
#endif
		va_copy(aq, ap);

		sz = evutil_vsnprintf(buffer, space, fmt, aq);

		va_end(aq);

		if (sz < 0)
			return (-1);
		if ((size_t)sz < space) {
			buf->off += sz;
			if (buf->cb != NULL)
				(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);
			return (sz);
		}
		if (evbuffer_expand(buf, sz + 1) == -1)
			return (-1);

	}
	/* NOTREACHED */
}