struct inet_peer *inet_getpeer(struct inetpeer_addr *daddr, int create)
{
	struct inet_peer __rcu **stack[PEER_MAXDEPTH], ***stackptr;
	struct inet_peer_base *base = family_to_base(daddr->family);
	struct inet_peer *p;
	unsigned int sequence;
	int invalidated, gccnt = 0;

	/* Attempt a lockless lookup first.
	 * Because of a concurrent writer, we might not find an existing entry.
	 */
	rcu_read_lock();
	sequence = read_seqbegin(&base->lock);
	p = lookup_rcu(daddr, base);
	invalidated = read_seqretry(&base->lock, sequence);
	rcu_read_unlock();

	if (p)
		return p;

	/* If no writer did a change during our lookup, we can return early. */
	if (!create && !invalidated)
		return NULL;

	/* retry an exact lookup, taking the lock before.
	 * At least, nodes should be hot in our cache.
	 */
	write_seqlock_bh(&base->lock);
relookup:
	p = lookup(daddr, stack, base);
	if (p != peer_avl_empty) {
		atomic_inc(&p->refcnt);
		write_sequnlock_bh(&base->lock);
		return p;
	}
	if (!gccnt) {
		gccnt = inet_peer_gc(base, stack, stackptr);
		if (gccnt && create)
			goto relookup;
	}
	p = create ? kmem_cache_alloc(peer_cachep, GFP_ATOMIC) : NULL;
	if (p) {
		p->daddr = *daddr;
		atomic_set(&p->refcnt, 1);
		atomic_set(&p->rid, 0);
		atomic_set(&p->ip_id_count, secure_ip_id(daddr->addr.a4));
		p->tcp_ts_stamp = 0;
		p->metrics[RTAX_LOCK-1] = INETPEER_METRICS_NEW;
		p->rate_tokens = 0;
		p->rate_last = 0;
		p->pmtu_expires = 0;
		p->pmtu_orig = 0;
		memset(&p->redirect_learned, 0, sizeof(p->redirect_learned));


		/* Link the node. */
		link_to_pool(p, base);
		base->total++;
	}
	write_sequnlock_bh(&base->lock);

	return p;
}