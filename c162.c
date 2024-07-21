static int jpc_ppt_getparms(jpc_ms_t *ms, jpc_cstate_t *cstate, jas_stream_t *in)
{
	jpc_ppt_t *ppt = &ms->parms.ppt;

	/* Eliminate compiler warning about unused variables. */
	cstate = 0;

	ppt->data = 0;

	if (ms->len < 1) {
		goto error;
	}
	if (jpc_getuint8(in, &ppt->ind)) {
		goto error;
	}
	ppt->len = ms->len - 1;
	if (ppt->len > 0) {
		if (!(ppt->data = jas_malloc(ppt->len * sizeof(unsigned char)))) {
			goto error;
		}
		if (jas_stream_read(in, (char *) ppt->data, ppt->len) != JAS_CAST(int, ppt->len)) {
			goto error;
		}
	} else {
		ppt->data = 0;
	}
	return 0;

error:
	jpc_ppt_destroyparms(ms);
	return -1;
}