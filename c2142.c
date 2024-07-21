evbuffer_copyout_from(struct evbuffer *buf, const struct evbuffer_ptr *pos,
    void *data_out, size_t datlen)
{
	/*XXX fails badly on sendfile case. */
	struct evbuffer_chain *chain;
	char *data = data_out;
	size_t nread;
	ev_ssize_t result = 0;
	size_t pos_in_chain;

	EVBUFFER_LOCK(buf);

	if (pos) {
		chain = pos->internal_.chain;
		pos_in_chain = pos->internal_.pos_in_chain;
		if (datlen + pos->pos > buf->total_len)
			datlen = buf->total_len - pos->pos;
	} else {
		chain = buf->first;
		pos_in_chain = 0;
		if (datlen > buf->total_len)
			datlen = buf->total_len;
	}


	if (datlen == 0)
		goto done;

	if (buf->freeze_start) {
		result = -1;
		goto done;
	}

	nread = datlen;

	while (datlen && datlen >= chain->off - pos_in_chain) {
		size_t copylen = chain->off - pos_in_chain;
		memcpy(data,
		    chain->buffer + chain->misalign + pos_in_chain,
		    copylen);
		data += copylen;
		datlen -= copylen;

		chain = chain->next;
		pos_in_chain = 0;
		EVUTIL_ASSERT(chain || datlen==0);
	}

	if (datlen) {
		EVUTIL_ASSERT(chain);
		memcpy(data, chain->buffer + chain->misalign + pos_in_chain,
		    datlen);
	}

	result = nread;
done:
	EVBUFFER_UNLOCK(buf);
	return result;
}