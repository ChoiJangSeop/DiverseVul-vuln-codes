static int jpc_cox_getcompparms(jpc_ms_t *ms, jpc_cstate_t *cstate,
  jas_stream_t *in, int prtflag, jpc_coxcp_t *compparms)
{
	uint_fast8_t tmp;
	int i;

	/* Eliminate compiler warning about unused variables. */
	ms = 0;
	cstate = 0;

	if (jpc_getuint8(in, &compparms->numdlvls) ||
	  jpc_getuint8(in, &compparms->cblkwidthval) ||
	  jpc_getuint8(in, &compparms->cblkheightval) ||
	  jpc_getuint8(in, &compparms->cblksty) ||
	  jpc_getuint8(in, &compparms->qmfbid)) {
		return -1;
	}
	if (compparms->numdlvls > 32) {
		goto error;
	}
	compparms->numrlvls = compparms->numdlvls + 1;
	if (compparms->numrlvls > JPC_MAXRLVLS) {
		goto error;
	}
	if (prtflag) {
		for (i = 0; i < compparms->numrlvls; ++i) {
			if (jpc_getuint8(in, &tmp)) {
				goto error;
			}
			compparms->rlvls[i].parwidthval = tmp & 0xf;
			compparms->rlvls[i].parheightval = (tmp >> 4) & 0xf;
		}
		/* Sigh.
		This bit should be in the same field in both COC and COD mrk segs. */
		compparms->csty |= JPC_COX_PRT;
	}
	if (jas_stream_eof(in)) {
		goto error;
	}
	return 0;
error:
	if (compparms) {
		jpc_cox_destroycompparms(compparms);
	}
	return -1;
}