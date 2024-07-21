int ida_get_new_above(struct ida *ida, int starting_id, int *p_id)
{
	struct idr_layer *pa[MAX_IDR_LEVEL];
	struct ida_bitmap *bitmap;
	unsigned long flags;
	int idr_id = starting_id / IDA_BITMAP_BITS;
	int offset = starting_id % IDA_BITMAP_BITS;
	int t, id;

 restart:
	/* get vacant slot */
	t = idr_get_empty_slot(&ida->idr, idr_id, pa, 0, &ida->idr);
	if (t < 0)
		return t == -ENOMEM ? -EAGAIN : t;

	if (t * IDA_BITMAP_BITS >= MAX_IDR_BIT)
		return -ENOSPC;

	if (t != idr_id)
		offset = 0;
	idr_id = t;

	/* if bitmap isn't there, create a new one */
	bitmap = (void *)pa[0]->ary[idr_id & IDR_MASK];
	if (!bitmap) {
		spin_lock_irqsave(&ida->idr.lock, flags);
		bitmap = ida->free_bitmap;
		ida->free_bitmap = NULL;
		spin_unlock_irqrestore(&ida->idr.lock, flags);

		if (!bitmap)
			return -EAGAIN;

		memset(bitmap, 0, sizeof(struct ida_bitmap));
		rcu_assign_pointer(pa[0]->ary[idr_id & IDR_MASK],
				(void *)bitmap);
		pa[0]->count++;
	}

	/* lookup for empty slot */
	t = find_next_zero_bit(bitmap->bitmap, IDA_BITMAP_BITS, offset);
	if (t == IDA_BITMAP_BITS) {
		/* no empty slot after offset, continue to the next chunk */
		idr_id++;
		offset = 0;
		goto restart;
	}

	id = idr_id * IDA_BITMAP_BITS + t;
	if (id >= MAX_IDR_BIT)
		return -ENOSPC;

	__set_bit(t, bitmap->bitmap);
	if (++bitmap->nr_busy == IDA_BITMAP_BITS)
		idr_mark_full(pa, idr_id);

	*p_id = id;

	/* Each leaf node can handle nearly a thousand slots and the
	 * whole idea of ida is to have small memory foot print.
	 * Throw away extra resources one by one after each successful
	 * allocation.
	 */
	if (ida->idr.id_free_cnt || ida->free_bitmap) {
		struct idr_layer *p = get_from_free_list(&ida->idr);
		if (p)
			kmem_cache_free(idr_layer_cache, p);
	}

	return 0;
}