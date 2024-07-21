static int jpc_poc_getparms(jpc_ms_t *ms, jpc_cstate_t *cstate, jas_stream_t *in)
{
	jpc_poc_t *poc = &ms->parms.poc;
	jpc_pocpchg_t *pchg;
	int pchgno;
	uint_fast8_t tmp;
	poc->numpchgs = (cstate->numcomps > 256) ? (ms->len / 9) :
	  (ms->len / 7);
	if (!(poc->pchgs = jas_malloc(poc->numpchgs * sizeof(jpc_pocpchg_t)))) {
		goto error;
	}
	for (pchgno = 0, pchg = poc->pchgs; pchgno < poc->numpchgs; ++pchgno,
	  ++pchg) {
		if (jpc_getuint8(in, &pchg->rlvlnostart)) {
			goto error;
		}
		if (cstate->numcomps > 256) {
			if (jpc_getuint16(in, &pchg->compnostart)) {
				goto error;
			}
		} else {
			if (jpc_getuint8(in, &tmp)) {
				goto error;
			};
			pchg->compnostart = tmp;
		}
		if (jpc_getuint16(in, &pchg->lyrnoend) ||
		  jpc_getuint8(in, &pchg->rlvlnoend)) {
			goto error;
		}
		if (cstate->numcomps > 256) {
			if (jpc_getuint16(in, &pchg->compnoend)) {
				goto error;
			}
		} else {
			if (jpc_getuint8(in, &tmp)) {
				goto error;
			}
			pchg->compnoend = tmp;
		}
		if (jpc_getuint8(in, &pchg->prgord)) {
			goto error;
		}
		if (pchg->rlvlnostart > pchg->rlvlnoend ||
		  pchg->compnostart > pchg->compnoend) {
			goto error;
		}
	}
	return 0;

error:
	jpc_poc_destroyparms(ms);
	return -1;
}