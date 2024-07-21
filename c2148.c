evbuffer_prepend(struct evbuffer *buf, const void *data, size_t datlen)
{
	struct evbuffer_chain *chain, *tmp;
	int result = -1;

	EVBUFFER_LOCK(buf);

	if (buf->freeze_start) {
		goto done;
	}

	chain = buf->first;

	if (chain == NULL) {
		chain = evbuffer_chain_new(datlen);
		if (!chain)
			goto done;
		evbuffer_chain_insert(buf, chain);
	}

	/* we cannot touch immutable buffers */
	if ((chain->flags & EVBUFFER_IMMUTABLE) == 0) {
		/* If this chain is empty, we can treat it as
		 * 'empty at the beginning' rather than 'empty at the end' */
		if (chain->off == 0)
			chain->misalign = chain->buffer_len;

		if ((size_t)chain->misalign >= datlen) {
			/* we have enough space to fit everything */
			memcpy(chain->buffer + chain->misalign - datlen,
			    data, datlen);
			chain->off += datlen;
			chain->misalign -= datlen;
			buf->total_len += datlen;
			buf->n_add_for_cb += datlen;
			goto out;
		} else if (chain->misalign) {
			/* we can only fit some of the data. */
			memcpy(chain->buffer,
			    (char*)data + datlen - chain->misalign,
			    (size_t)chain->misalign);
			chain->off += (size_t)chain->misalign;
			buf->total_len += (size_t)chain->misalign;
			buf->n_add_for_cb += (size_t)chain->misalign;
			datlen -= (size_t)chain->misalign;
			chain->misalign = 0;
		}
	}

	/* we need to add another chain */
	if ((tmp = evbuffer_chain_new(datlen)) == NULL)
		goto done;
	buf->first = tmp;
	if (buf->last_with_datap == &buf->first)
		buf->last_with_datap = &tmp->next;

	tmp->next = chain;

	tmp->off = datlen;
	tmp->misalign = tmp->buffer_len - datlen;

	memcpy(tmp->buffer + tmp->misalign, data, datlen);
	buf->total_len += datlen;
	buf->n_add_for_cb += (size_t)chain->misalign;

out:
	evbuffer_invoke_callbacks_(buf);
	result = 0;
done:
	EVBUFFER_UNLOCK(buf);
	return result;
}