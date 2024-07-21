static int jpc_unk_getparms(jpc_ms_t *ms, jpc_cstate_t *cstate, jas_stream_t *in)
{
	jpc_unk_t *unk = &ms->parms.unk;

	/* Eliminate compiler warning about unused variables. */
	cstate = 0;

	if (ms->len > 0) {
		if (!(unk->data = jas_malloc(ms->len * sizeof(unsigned char)))) {
			return -1;
		}
		if (jas_stream_read(in, (char *) unk->data, ms->len) != JAS_CAST(int, ms->len)) {
			jas_free(unk->data);
			return -1;
		}
		unk->len = ms->len;
	} else {
		unk->data = 0;
		unk->len = 0;
	}
	return 0;
}