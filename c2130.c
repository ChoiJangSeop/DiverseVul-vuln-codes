evbuffer_ptr_set(struct evbuffer *buf, struct evbuffer_ptr *pos,
    size_t position, enum evbuffer_ptr_how how)
{
	size_t left = position;
	struct evbuffer_chain *chain = NULL;

	EVBUFFER_LOCK(buf);

	switch (how) {
	case EVBUFFER_PTR_SET:
		chain = buf->first;
		pos->pos = position;
		position = 0;
		break;
	case EVBUFFER_PTR_ADD:
		/* this avoids iterating over all previous chains if
		   we just want to advance the position */
		chain = pos->_internal.chain;
		pos->pos += position;
		position = pos->_internal.pos_in_chain;
		break;
	}

	while (chain && position + left >= chain->off) {
		left -= chain->off - position;
		chain = chain->next;
		position = 0;
	}
	if (chain) {
		pos->_internal.chain = chain;
		pos->_internal.pos_in_chain = position + left;
	} else {
		pos->_internal.chain = NULL;
		pos->pos = -1;
	}

	EVBUFFER_UNLOCK(buf);

	return chain != NULL ? 0 : -1;
}