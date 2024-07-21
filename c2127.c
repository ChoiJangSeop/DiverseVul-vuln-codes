evbuffer_copyout(struct evbuffer *buf, void *data_out, size_t datlen)
{
	/*XXX fails badly on sendfile case. */
	struct evbuffer_chain *chain;
	char *data = data_out;
	size_t nread;
	ev_ssize_t result = 0;

	EVBUFFER_LOCK(buf);

	chain = buf->first;

	if (datlen >= buf->total_len)
		datlen = buf->total_len;

	if (datlen == 0)
		goto done;

	if (buf->freeze_start) {
		result = -1;
		goto done;
	}

	nread = datlen;

	while (datlen && datlen >= chain->off) {
		memcpy(data, chain->buffer + chain->misalign, chain->off);
		data += chain->off;
		datlen -= chain->off;

		chain = chain->next;
		EVUTIL_ASSERT(chain || datlen==0);
	}

	if (datlen) {
		EVUTIL_ASSERT(chain);
		memcpy(data, chain->buffer + chain->misalign, datlen);
	}

	result = nread;
done:
	EVBUFFER_UNLOCK(buf);
	return result;
}