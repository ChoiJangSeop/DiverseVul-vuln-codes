void *idr_get_next(struct idr *idp, int *nextidp)
{
	struct idr_layer *p, *pa[MAX_IDR_LEVEL];
	struct idr_layer **paa = &pa[0];
	int id = *nextidp;
	int n, max;

	/* find first ent */
	p = rcu_dereference_raw(idp->top);
	if (!p)
		return NULL;
	n = (p->layer + 1) * IDR_BITS;
	max = 1 << n;

	while (id < max) {
		while (n > 0 && p) {
			n -= IDR_BITS;
			*paa++ = p;
			p = rcu_dereference_raw(p->ary[(id >> n) & IDR_MASK]);
		}

		if (p) {
			*nextidp = id;
			return p;
		}

		/*
		 * Proceed to the next layer at the current level.  Unlike
		 * idr_for_each(), @id isn't guaranteed to be aligned to
		 * layer boundary at this point and adding 1 << n may
		 * incorrectly skip IDs.  Make sure we jump to the
		 * beginning of the next layer using round_up().
		 */
		id = round_up(id + 1, 1 << n);
		while (n < fls(id)) {
			n += IDR_BITS;
			p = *--paa;
		}
	}
	return NULL;
}