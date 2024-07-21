static jpc_dec_cp_t *jpc_dec_cp_create(uint_fast16_t numcomps)
{
	jpc_dec_cp_t *cp;
	jpc_dec_ccp_t *ccp;
	int compno;

	if (!(cp = jas_malloc(sizeof(jpc_dec_cp_t)))) {
		return 0;
	}
	cp->flags = 0;
	cp->numcomps = numcomps;
	cp->prgord = 0;
	cp->numlyrs = 0;
	cp->mctid = 0;
	cp->csty = 0;
	if (!(cp->ccps = jas_malloc(cp->numcomps * sizeof(jpc_dec_ccp_t)))) {
		return 0;
	}
	if (!(cp->pchglist = jpc_pchglist_create())) {
		jas_free(cp->ccps);
		return 0;
	}
	for (compno = 0, ccp = cp->ccps; compno < cp->numcomps;
	  ++compno, ++ccp) {
		ccp->flags = 0;
		ccp->numrlvls = 0;
		ccp->cblkwidthexpn = 0;
		ccp->cblkheightexpn = 0;
		ccp->qmfbid = 0;
		ccp->numstepsizes = 0;
		ccp->numguardbits = 0;
		ccp->roishift = 0;
		ccp->cblkctx = 0;
	}
	return cp;
}