evbuffer_ptr_set(struct evbuffer *buf, struct evbuffer_ptr *pos,
    size_t position, enum evbuffer_ptr_how how)
{
	size_t left = position;
	struct evbuffer_chain *chain = NULL;
	int result = 0;

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
		chain = pos->internal_.chain;
		pos->pos += position;
		position = pos->internal_.pos_in_chain;
		break;
	}

	while (chain && position + left >= chain->off) {
		left -= chain->off - position;
		chain = chain->next;
		position = 0;
	}
	if (chain) {
		pos->internal_.chain = chain;
		pos->internal_.pos_in_chain = position + left;
	} else if (left == 0) {
		/* The first byte in the (nonexistent) chain after the last chain */
		pos->internal_.chain = NULL;
		pos->internal_.pos_in_chain = 0;
	} else {
		PTR_NOT_FOUND(pos);
		result = -1;
	}

	EVBUFFER_UNLOCK(buf);

	return result;
}