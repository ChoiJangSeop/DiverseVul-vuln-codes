int idr_get_new_above(struct idr *idp, void *ptr, int starting_id, int *id)
{
	struct idr_layer *pa[MAX_IDR_LEVEL];
	int rv;

	rv = idr_get_empty_slot(idp, starting_id, pa, 0, idp);
	if (rv < 0)
		return rv == -ENOMEM ? -EAGAIN : rv;

	idr_fill_slot(ptr, rv, pa);
	*id = rv;
	return 0;
}