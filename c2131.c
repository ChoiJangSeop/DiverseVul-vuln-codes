evbuffer_drain(struct evbuffer *buf, size_t len)
{
	struct evbuffer_chain *chain, *next;
	size_t remaining, old_len;
	int result = 0;

	EVBUFFER_LOCK(buf);
	old_len = buf->total_len;

	if (old_len == 0)
		goto done;

	if (buf->freeze_start) {
		result = -1;
		goto done;
	}

	if (len >= old_len && !HAS_PINNED_R(buf)) {
		len = old_len;
		for (chain = buf->first; chain != NULL; chain = next) {
			next = chain->next;
			evbuffer_chain_free(chain);
		}

		ZERO_CHAIN(buf);
	} else {
		if (len >= old_len)
			len = old_len;

		buf->total_len -= len;
		remaining = len;
		for (chain = buf->first;
		     remaining >= chain->off;
		     chain = next) {
			next = chain->next;
			remaining -= chain->off;

			if (chain == *buf->last_with_datap) {
				buf->last_with_datap = &buf->first;
			}
			if (&chain->next == buf->last_with_datap)
				buf->last_with_datap = &buf->first;

			if (CHAIN_PINNED_R(chain)) {
				EVUTIL_ASSERT(remaining == 0);
				chain->misalign += chain->off;
				chain->off = 0;
				break;
			} else
				evbuffer_chain_free(chain);
		}

		buf->first = chain;
		if (chain) {
			chain->misalign += remaining;
			chain->off -= remaining;
		}
	}

	buf->n_del_for_cb += len;
	/* Tell someone about changes in this buffer */
	evbuffer_invoke_callbacks(buf);

done:
	EVBUFFER_UNLOCK(buf);
	return result;
}