static int jpc_qcx_getcompparms(jpc_qcxcp_t *compparms, jpc_cstate_t *cstate,
  jas_stream_t *in, uint_fast16_t len)
{
	uint_fast8_t tmp;
	int n;
	int i;

	/* Eliminate compiler warning about unused variables. */
	cstate = 0;

	n = 0;
	if (jpc_getuint8(in, &tmp)) {
		return -1;
	}
	++n;
	compparms->qntsty = tmp & 0x1f;
	compparms->numguard = (tmp >> 5) & 7;
	switch (compparms->qntsty) {
	case JPC_QCX_SIQNT:
		compparms->numstepsizes = 1;
		break;
	case JPC_QCX_NOQNT:
		compparms->numstepsizes = (len - n);
		break;
	case JPC_QCX_SEQNT:
		/* XXX - this is a hack */
		compparms->numstepsizes = (len - n) / 2;
		break;
	}
	if (compparms->numstepsizes > 0) {
		compparms->stepsizes = jas_malloc(compparms->numstepsizes *
		  sizeof(uint_fast16_t));
		assert(compparms->stepsizes);
		for (i = 0; i < compparms->numstepsizes; ++i) {
			if (compparms->qntsty == JPC_QCX_NOQNT) {
				if (jpc_getuint8(in, &tmp)) {
					return -1;
				}
				compparms->stepsizes[i] = JPC_QCX_EXPN(tmp >> 3);
			} else {
				if (jpc_getuint16(in, &compparms->stepsizes[i])) {
					return -1;
				}
			}
		}
	} else {
		compparms->stepsizes = 0;
	}
	if (jas_stream_error(in) || jas_stream_eof(in)) {
		jpc_qcx_destroycompparms(compparms);
		return -1;
	}
	return 0;
}