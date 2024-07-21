static void huft_free(huft_t *p)
{
	huft_t *q;

	/* Go through linked list, freeing from the malloced (t[-1]) address. */
	while (p) {
		q = (--p)->v.t;
		free(p);
		p = q;
	}
}