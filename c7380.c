int idr_alloc(struct idr *idr, void *ptr, int start, int end, gfp_t gfp_mask)
{
	int max = end > 0 ? end - 1 : INT_MAX;	/* inclusive upper limit */
	struct idr_layer *pa[MAX_IDR_LEVEL];
	int id;

	might_sleep_if(gfp_mask & __GFP_WAIT);

	/* sanity checks */
	if (WARN_ON_ONCE(start < 0))
		return -EINVAL;
	if (unlikely(max < start))
		return -ENOSPC;

	/* allocate id */
	id = idr_get_empty_slot(idr, start, pa, gfp_mask, NULL);
	if (unlikely(id < 0))
		return id;
	if (unlikely(id > max))
		return -ENOSPC;

	idr_fill_slot(ptr, id, pa);
	return id;
}