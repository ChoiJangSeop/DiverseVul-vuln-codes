evbuffer_ptr_subtract(struct evbuffer *buf, struct evbuffer_ptr *pos,
    size_t howfar)
{
	if (howfar > (size_t)pos->pos)
		return -1;
	if (pos->internal_.chain && howfar <= pos->internal_.pos_in_chain) {
		pos->internal_.pos_in_chain -= howfar;
		pos->pos -= howfar;
		return 0;
	} else {
		const size_t newpos = pos->pos - howfar;
		/* Here's the inefficient part: it walks over the
		 * chains until we hit newpos. */
		return evbuffer_ptr_set(buf, pos, newpos, EVBUFFER_PTR_SET);
	}
}