void unix_gc(void)
{
	static bool gc_in_progress = false;

	struct unix_sock *u;
	struct unix_sock *next;
	struct sk_buff_head hitlist;
	struct list_head cursor;

	spin_lock(&unix_gc_lock);

	/* Avoid a recursive GC. */
	if (gc_in_progress)
		goto out;

	gc_in_progress = true;
	/*
	 * First, select candidates for garbage collection.  Only
	 * in-flight sockets are considered, and from those only ones
	 * which don't have any external reference.
	 *
	 * Holding unix_gc_lock will protect these candidates from
	 * being detached, and hence from gaining an external
	 * reference.  This also means, that since there are no
	 * possible receivers, the receive queues of these sockets are
	 * static during the GC, even though the dequeue is done
	 * before the detach without atomicity guarantees.
	 */
	list_for_each_entry_safe(u, next, &gc_inflight_list, link) {
		long total_refs;
		long inflight_refs;

		total_refs = file_count(u->sk.sk_socket->file);
		inflight_refs = atomic_long_read(&u->inflight);

		BUG_ON(inflight_refs < 1);
		BUG_ON(total_refs < inflight_refs);
		if (total_refs == inflight_refs) {
			list_move_tail(&u->link, &gc_candidates);
			u->gc_candidate = 1;
		}
	}

	/*
	 * Now remove all internal in-flight reference to children of
	 * the candidates.
	 */
	list_for_each_entry(u, &gc_candidates, link)
		scan_children(&u->sk, dec_inflight, NULL);

	/*
	 * Restore the references for children of all candidates,
	 * which have remaining references.  Do this recursively, so
	 * only those remain, which form cyclic references.
	 *
	 * Use a "cursor" link, to make the list traversal safe, even
	 * though elements might be moved about.
	 */
	list_add(&cursor, &gc_candidates);
	while (cursor.next != &gc_candidates) {
		u = list_entry(cursor.next, struct unix_sock, link);

		/* Move cursor to after the current position. */
		list_move(&cursor, &u->link);

		if (atomic_long_read(&u->inflight) > 0) {
			list_move_tail(&u->link, &gc_inflight_list);
			u->gc_candidate = 0;
			scan_children(&u->sk, inc_inflight_move_tail, NULL);
		}
	}
	list_del(&cursor);

	/*
	 * Now gc_candidates contains only garbage.  Restore original
	 * inflight counters for these as well, and remove the skbuffs
	 * which are creating the cycle(s).
	 */
	skb_queue_head_init(&hitlist);
	list_for_each_entry(u, &gc_candidates, link)
		scan_children(&u->sk, inc_inflight, &hitlist);

	spin_unlock(&unix_gc_lock);

	/* Here we are. Hitlist is filled. Die. */
	__skb_queue_purge(&hitlist);

	spin_lock(&unix_gc_lock);

	/* All candidates should have been detached by now. */
	BUG_ON(!list_empty(&gc_candidates));
	gc_in_progress = false;

 out:
	spin_unlock(&unix_gc_lock);
}