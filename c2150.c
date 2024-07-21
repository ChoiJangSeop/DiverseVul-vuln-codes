evbuffer_expand_fast_(struct evbuffer *buf, size_t datlen, int n)
{
	struct evbuffer_chain *chain = buf->last, *tmp, *next;
	size_t avail;
	int used;

	ASSERT_EVBUFFER_LOCKED(buf);
	EVUTIL_ASSERT(n >= 2);

	if (chain == NULL || (chain->flags & EVBUFFER_IMMUTABLE)) {
		/* There is no last chunk, or we can't touch the last chunk.
		 * Just add a new chunk. */
		chain = evbuffer_chain_new(datlen);
		if (chain == NULL)
			return (-1);

		evbuffer_chain_insert(buf, chain);
		return (0);
	}

	used = 0; /* number of chains we're using space in. */
	avail = 0; /* how much space they have. */
	/* How many bytes can we stick at the end of buffer as it is?  Iterate
	 * over the chains at the end of the buffer, tring to see how much
	 * space we have in the first n. */
	for (chain = *buf->last_with_datap; chain; chain = chain->next) {
		if (chain->off) {
			size_t space = (size_t) CHAIN_SPACE_LEN(chain);
			EVUTIL_ASSERT(chain == *buf->last_with_datap);
			if (space) {
				avail += space;
				++used;
			}
		} else {
			/* No data in chain; realign it. */
			chain->misalign = 0;
			avail += chain->buffer_len;
			++used;
		}
		if (avail >= datlen) {
			/* There is already enough space.  Just return */
			return (0);
		}
		if (used == n)
			break;
	}

	/* There wasn't enough space in the first n chains with space in
	 * them. Either add a new chain with enough space, or replace all
	 * empty chains with one that has enough space, depending on n. */
	if (used < n) {
		/* The loop ran off the end of the chains before it hit n
		 * chains; we can add another. */
		EVUTIL_ASSERT(chain == NULL);

		tmp = evbuffer_chain_new(datlen - avail);
		if (tmp == NULL)
			return (-1);

		buf->last->next = tmp;
		buf->last = tmp;
		/* (we would only set last_with_data if we added the first
		 * chain. But if the buffer had no chains, we would have
		 * just allocated a new chain earlier) */
		return (0);
	} else {
		/* Nuke _all_ the empty chains. */
		int rmv_all = 0; /* True iff we removed last_with_data. */
		chain = *buf->last_with_datap;
		if (!chain->off) {
			EVUTIL_ASSERT(chain == buf->first);
			rmv_all = 1;
			avail = 0;
		} else {
			avail = (size_t) CHAIN_SPACE_LEN(chain);
			chain = chain->next;
		}


		for (; chain; chain = next) {
			next = chain->next;
			EVUTIL_ASSERT(chain->off == 0);
			evbuffer_chain_free(chain);
		}
		tmp = evbuffer_chain_new(datlen - avail);
		if (tmp == NULL) {
			if (rmv_all) {
				ZERO_CHAIN(buf);
			} else {
				buf->last = *buf->last_with_datap;
				(*buf->last_with_datap)->next = NULL;
			}
			return (-1);
		}

		if (rmv_all) {
			buf->first = buf->last = tmp;
			buf->last_with_datap = &buf->first;
		} else {
			(*buf->last_with_datap)->next = tmp;
			buf->last = tmp;
		}
		return (0);
	}
}