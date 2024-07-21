jpc_mqdec_t *jpc_mqdec_create(int maxctxs, jas_stream_t *in)
{
	jpc_mqdec_t *mqdec;

	/* There must be at least one context. */
	assert(maxctxs > 0);

	/* Allocate memory for the MQ decoder. */
	if (!(mqdec = jas_malloc(sizeof(jpc_mqdec_t)))) {
		goto error;
	}
	mqdec->in = in;
	mqdec->maxctxs = maxctxs;
	/* Allocate memory for the per-context state information. */
	if (!(mqdec->ctxs = jas_malloc(mqdec->maxctxs * sizeof(jpc_mqstate_t *)))) {
		goto error;
	}
	/* Set the current context to the first context. */
	mqdec->curctx = mqdec->ctxs;

	/* If an input stream has been associated with the MQ decoder,
	  initialize the decoder state from the stream. */
	if (mqdec->in) {
		jpc_mqdec_init(mqdec);
	}
	/* Initialize the per-context state information. */
	jpc_mqdec_setctxs(mqdec, 0, 0);

	return mqdec;

error:
	/* Oops...  Something has gone wrong. */
	if (mqdec) {
		jpc_mqdec_destroy(mqdec);
	}
	return 0;
}