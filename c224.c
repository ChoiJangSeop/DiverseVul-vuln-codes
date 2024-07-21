static void inc_inflight_move_tail(struct unix_sock *u)
{
	atomic_long_inc(&u->inflight);
	/*
	 * If this is still a candidate, move it to the end of the
	 * list, so that it's checked even if it was already passed
	 * over
	 */
	if (u->gc_candidate)
		list_move_tail(&u->link, &gc_candidates);
}