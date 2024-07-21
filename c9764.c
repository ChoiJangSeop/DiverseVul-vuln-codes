int kmem_cache_alloc_bulk(struct kmem_cache *s, gfp_t flags, size_t size,
			  void **p)
{
	struct kmem_cache_cpu *c;
	int i;

	/* memcg and kmem_cache debug support */
	s = slab_pre_alloc_hook(s, flags);
	if (unlikely(!s))
		return false;
	/*
	 * Drain objects in the per cpu slab, while disabling local
	 * IRQs, which protects against PREEMPT and interrupts
	 * handlers invoking normal fastpath.
	 */
	local_irq_disable();
	c = this_cpu_ptr(s->cpu_slab);

	for (i = 0; i < size; i++) {
		void *object = c->freelist;

		if (unlikely(!object)) {
			/*
			 * Invoking slow path likely have side-effect
			 * of re-populating per CPU c->freelist
			 */
			p[i] = ___slab_alloc(s, flags, NUMA_NO_NODE,
					    _RET_IP_, c);
			if (unlikely(!p[i]))
				goto error;

			c = this_cpu_ptr(s->cpu_slab);
			maybe_wipe_obj_freeptr(s, p[i]);

			continue; /* goto for-loop */
		}
		c->freelist = get_freepointer(s, object);
		p[i] = object;
		maybe_wipe_obj_freeptr(s, p[i]);
	}
	c->tid = next_tid(c->tid);
	local_irq_enable();

	/* Clear memory outside IRQ disabled fastpath loop */
	if (unlikely(slab_want_init_on_alloc(flags, s))) {
		int j;

		for (j = 0; j < i; j++)
			memset(p[j], 0, s->object_size);
	}

	/* memcg and kmem_cache debug support */
	slab_post_alloc_hook(s, flags, size, p);
	return i;
error:
	local_irq_enable();
	slab_post_alloc_hook(s, flags, i, p);
	__kmem_cache_free_bulk(s, i, p);
	return 0;
}