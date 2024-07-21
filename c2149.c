evbuffer_ptr_memcmp(const struct evbuffer *buf, const struct evbuffer_ptr *pos,
    const char *mem, size_t len)
{
	struct evbuffer_chain *chain;
	size_t position;
	int r;

	ASSERT_EVBUFFER_LOCKED(buf);

	if (pos->pos + len > buf->total_len)
		return -1;

	chain = pos->internal_.chain;
	position = pos->internal_.pos_in_chain;
	while (len && chain) {
		size_t n_comparable;
		if (len + position > chain->off)
			n_comparable = chain->off - position;
		else
			n_comparable = len;
		r = memcmp(chain->buffer + chain->misalign + position, mem,
		    n_comparable);
		if (r)
			return r;
		mem += n_comparable;
		len -= n_comparable;
		position = 0;
		chain = chain->next;
	}

	return 0;
}