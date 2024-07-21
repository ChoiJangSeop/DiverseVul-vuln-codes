evbuffer_expand_singlechain(struct evbuffer *buf, size_t datlen)
{
	struct evbuffer_chain *chain, **chainp;
	struct evbuffer_chain *result = NULL;
	ASSERT_EVBUFFER_LOCKED(buf);

	chainp = buf->last_with_datap;

	/* XXX If *chainp is no longer writeable, but has enough space in its
	 * misalign, this might be a bad idea: we could still use *chainp, not
	 * (*chainp)->next. */
	if (*chainp && CHAIN_SPACE_LEN(*chainp) == 0)
		chainp = &(*chainp)->next;

	/* 'chain' now points to the first chain with writable space (if any)
	 * We will either use it, realign it, replace it, or resize it. */
	chain = *chainp;

	if (chain == NULL ||
	    (chain->flags & (EVBUFFER_IMMUTABLE|EVBUFFER_MEM_PINNED_ANY))) {
		/* We can't use the last_with_data chain at all.  Just add a
		 * new one that's big enough. */
		goto insert_new;
	}

	/* If we can fit all the data, then we don't have to do anything */
	if (CHAIN_SPACE_LEN(chain) >= datlen) {
		result = chain;
		goto ok;
	}

	/* If the chain is completely empty, just replace it by adding a new
	 * empty chain. */
	if (chain->off == 0) {
		goto insert_new;
	}

	/* If the misalignment plus the remaining space fulfills our data
	 * needs, we could just force an alignment to happen.  Afterwards, we
	 * have enough space.  But only do this if we're saving a lot of space
	 * and not moving too much data.  Otherwise the space savings are
	 * probably offset by the time lost in copying.
	 */
	if (evbuffer_chain_should_realign(chain, datlen)) {
		evbuffer_chain_align(chain);
		result = chain;
		goto ok;
	}

	/* At this point, we can either resize the last chunk with space in
	 * it, use the next chunk after it, or   If we add a new chunk, we waste
	 * CHAIN_SPACE_LEN(chain) bytes in the former last chunk.  If we
	 * resize, we have to copy chain->off bytes.
	 */

	/* Would expanding this chunk be affordable and worthwhile? */
	if (CHAIN_SPACE_LEN(chain) < chain->buffer_len / 8 ||
	    chain->off > MAX_TO_COPY_IN_EXPAND) {
		/* It's not worth resizing this chain. Can the next one be
		 * used? */
		if (chain->next && CHAIN_SPACE_LEN(chain->next) >= datlen) {
			/* Yes, we can just use the next chain (which should
			 * be empty. */
			result = chain->next;
			goto ok;
		} else {
			/* No; append a new chain (which will free all
			 * terminal empty chains.) */
			goto insert_new;
		}
	} else {
		/* Okay, we're going to try to resize this chain: Not doing so
		 * would waste at least 1/8 of its current allocation, and we
		 * can do so without having to copy more than
		 * MAX_TO_COPY_IN_EXPAND bytes. */
		/* figure out how much space we need */
		size_t length = chain->off + datlen;
		struct evbuffer_chain *tmp = evbuffer_chain_new(length);
		if (tmp == NULL)
			goto err;

		/* copy the data over that we had so far */
		tmp->off = chain->off;
		memcpy(tmp->buffer, chain->buffer + chain->misalign,
		    chain->off);
		/* fix up the list */
		EVUTIL_ASSERT(*chainp == chain);
		result = *chainp = tmp;

		if (buf->last == chain)
			buf->last = tmp;

		tmp->next = chain->next;
		evbuffer_chain_free(chain);
		goto ok;
	}

insert_new:
	result = evbuffer_chain_insert_new(buf, datlen);
	if (!result)
		goto err;
ok:
	EVUTIL_ASSERT(result);
	EVUTIL_ASSERT(CHAIN_SPACE_LEN(result) >= datlen);
err:
	return result;
}