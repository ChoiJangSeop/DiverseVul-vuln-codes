static int idr_get_empty_slot(struct idr *idp, int starting_id,
			      struct idr_layer **pa, gfp_t gfp_mask,
			      struct idr *layer_idr)
{
	struct idr_layer *p, *new;
	int layers, v, id;
	unsigned long flags;

	id = starting_id;
build_up:
	p = idp->top;
	layers = idp->layers;
	if (unlikely(!p)) {
		if (!(p = idr_layer_alloc(gfp_mask, layer_idr)))
			return -ENOMEM;
		p->layer = 0;
		layers = 1;
	}
	/*
	 * Add a new layer to the top of the tree if the requested
	 * id is larger than the currently allocated space.
	 */
	while ((layers < (MAX_IDR_LEVEL - 1)) && (id >= (1 << (layers*IDR_BITS)))) {
		layers++;
		if (!p->count) {
			/* special case: if the tree is currently empty,
			 * then we grow the tree by moving the top node
			 * upwards.
			 */
			p->layer++;
			continue;
		}
		if (!(new = idr_layer_alloc(gfp_mask, layer_idr))) {
			/*
			 * The allocation failed.  If we built part of
			 * the structure tear it down.
			 */
			spin_lock_irqsave(&idp->lock, flags);
			for (new = p; p && p != idp->top; new = p) {
				p = p->ary[0];
				new->ary[0] = NULL;
				new->bitmap = new->count = 0;
				__move_to_free_list(idp, new);
			}
			spin_unlock_irqrestore(&idp->lock, flags);
			return -ENOMEM;
		}
		new->ary[0] = p;
		new->count = 1;
		new->layer = layers-1;
		if (p->bitmap == IDR_FULL)
			__set_bit(0, &new->bitmap);
		p = new;
	}
	rcu_assign_pointer(idp->top, p);
	idp->layers = layers;
	v = sub_alloc(idp, &id, pa, gfp_mask, layer_idr);
	if (v == -EAGAIN)
		goto build_up;
	return(v);
}