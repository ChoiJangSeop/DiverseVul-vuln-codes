jpc_mqenc_t *jpc_mqenc_create(int maxctxs, jas_stream_t *out)
{
	jpc_mqenc_t *mqenc;

	/* Allocate memory for the MQ encoder. */
	if (!(mqenc = jas_malloc(sizeof(jpc_mqenc_t)))) {
		goto error;
	}
	mqenc->out = out;
	mqenc->maxctxs = maxctxs;

	/* Allocate memory for the per-context state information. */
	if (!(mqenc->ctxs = jas_malloc(mqenc->maxctxs * sizeof(jpc_mqstate_t *)))) {
		goto error;
	}

	/* Set the current context to the first one. */
	mqenc->curctx = mqenc->ctxs;

	jpc_mqenc_init(mqenc);

	/* Initialize the per-context state information to something sane. */
	jpc_mqenc_setctxs(mqenc, 0, 0);

	return mqenc;

error:
	if (mqenc) {
		jpc_mqenc_destroy(mqenc);
	}
	return 0;
}