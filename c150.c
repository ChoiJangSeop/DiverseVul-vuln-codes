static jpc_enc_rlvl_t *rlvl_create(jpc_enc_rlvl_t *rlvl, jpc_enc_cp_t *cp,
  jpc_enc_tcmpt_t *tcmpt, jpc_tsfb_band_t *bandinfos)
{
	uint_fast16_t rlvlno;
	uint_fast32_t tlprctlx;
	uint_fast32_t tlprctly;
	uint_fast32_t brprcbrx;
	uint_fast32_t brprcbry;
	uint_fast16_t bandno;
	jpc_enc_band_t *band;

	/* Deduce the resolution level. */
	rlvlno = rlvl - tcmpt->rlvls;

	/* Initialize members required for error recovery. */
	rlvl->bands = 0;
	rlvl->tcmpt = tcmpt;

	/* Compute the coordinates of the top-left and bottom-right
	  corners of the tile-component at this resolution. */
	rlvl->tlx = JPC_CEILDIVPOW2(jas_seq2d_xstart(tcmpt->data), tcmpt->numrlvls -
	  1 - rlvlno);
	rlvl->tly = JPC_CEILDIVPOW2(jas_seq2d_ystart(tcmpt->data), tcmpt->numrlvls -
	  1 - rlvlno);
	rlvl->brx = JPC_CEILDIVPOW2(jas_seq2d_xend(tcmpt->data), tcmpt->numrlvls -
	  1 - rlvlno);
	rlvl->bry = JPC_CEILDIVPOW2(jas_seq2d_yend(tcmpt->data), tcmpt->numrlvls -
	  1 - rlvlno);

	if (rlvl->tlx >= rlvl->brx || rlvl->tly >= rlvl->bry) {
		rlvl->numhprcs = 0;
		rlvl->numvprcs = 0;
		rlvl->numprcs = 0;
		return rlvl;
	}

	rlvl->numbands = (!rlvlno) ? 1 : 3;
	rlvl->prcwidthexpn = cp->tccp.prcwidthexpns[rlvlno];
	rlvl->prcheightexpn = cp->tccp.prcheightexpns[rlvlno];
	if (!rlvlno) {
		rlvl->cbgwidthexpn = rlvl->prcwidthexpn;
		rlvl->cbgheightexpn = rlvl->prcheightexpn;
	} else {
		rlvl->cbgwidthexpn = rlvl->prcwidthexpn - 1;
		rlvl->cbgheightexpn = rlvl->prcheightexpn - 1;
	}
	rlvl->cblkwidthexpn = JAS_MIN(cp->tccp.cblkwidthexpn, rlvl->cbgwidthexpn);
	rlvl->cblkheightexpn = JAS_MIN(cp->tccp.cblkheightexpn, rlvl->cbgheightexpn);

	/* Compute the number of precincts. */
	tlprctlx = JPC_FLOORTOMULTPOW2(rlvl->tlx, rlvl->prcwidthexpn);
	tlprctly = JPC_FLOORTOMULTPOW2(rlvl->tly, rlvl->prcheightexpn);
	brprcbrx = JPC_CEILTOMULTPOW2(rlvl->brx, rlvl->prcwidthexpn);
	brprcbry = JPC_CEILTOMULTPOW2(rlvl->bry, rlvl->prcheightexpn);
	rlvl->numhprcs = JPC_FLOORDIVPOW2(brprcbrx - tlprctlx, rlvl->prcwidthexpn);
	rlvl->numvprcs = JPC_FLOORDIVPOW2(brprcbry - tlprctly, rlvl->prcheightexpn);
	rlvl->numprcs = rlvl->numhprcs * rlvl->numvprcs;

	if (!(rlvl->bands = jas_malloc(rlvl->numbands * sizeof(jpc_enc_band_t)))) {
		goto error;
	}
	for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
	  ++bandno, ++band) {
		band->prcs = 0;
		band->data = 0;
		band->rlvl = rlvl;
	}
	for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
	  ++bandno, ++band) {
		if (!band_create(band, cp, rlvl, bandinfos)) {
			goto error;
		}
	}

	return rlvl;
error:

	rlvl_destroy(rlvl);
	return 0;
}