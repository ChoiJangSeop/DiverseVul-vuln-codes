static jpc_enc_tcmpt_t *tcmpt_create(jpc_enc_tcmpt_t *tcmpt, jpc_enc_cp_t *cp,
  jas_image_t *image, jpc_enc_tile_t *tile)
{
	uint_fast16_t cmptno;
	uint_fast16_t rlvlno;
	jpc_enc_rlvl_t *rlvl;
	uint_fast32_t tlx;
	uint_fast32_t tly;
	uint_fast32_t brx;
	uint_fast32_t bry;
	uint_fast32_t cmpttlx;
	uint_fast32_t cmpttly;
	jpc_enc_ccp_t *ccp;
	jpc_tsfb_band_t bandinfos[JPC_MAXBANDS];

	tcmpt->tile = tile;
	tcmpt->tsfb = 0;
	tcmpt->data = 0;
	tcmpt->rlvls = 0;

	/* Deduce the component number. */
	cmptno = tcmpt - tile->tcmpts;

	ccp = &cp->ccps[cmptno];

	/* Compute the coordinates of the top-left and bottom-right
	  corners of this tile-component. */
	tlx = JPC_CEILDIV(tile->tlx, ccp->sampgrdstepx);
	tly = JPC_CEILDIV(tile->tly, ccp->sampgrdstepy);
	brx = JPC_CEILDIV(tile->brx, ccp->sampgrdstepx);
	bry = JPC_CEILDIV(tile->bry, ccp->sampgrdstepy);

	/* Create a sequence to hold the tile-component sample data. */
	if (!(tcmpt->data = jas_seq2d_create(tlx, tly, brx, bry))) {
		goto error;
	}

	/* Get the image data associated with this tile-component. */
	cmpttlx = JPC_CEILDIV(cp->imgareatlx, ccp->sampgrdstepx);
	cmpttly = JPC_CEILDIV(cp->imgareatly, ccp->sampgrdstepy);
	if (jas_image_readcmpt(image, cmptno, tlx - cmpttlx, tly - cmpttly,
	  brx - tlx, bry - tly, tcmpt->data)) {
		goto error;
	}

	tcmpt->synweight = 0;
	tcmpt->qmfbid = cp->tccp.qmfbid;
	tcmpt->numrlvls = cp->tccp.maxrlvls;
	tcmpt->numbands = 3 * tcmpt->numrlvls - 2;
	if (!(tcmpt->tsfb = jpc_cod_gettsfb(tcmpt->qmfbid, tcmpt->numrlvls - 1))) {
		goto error;
	}

	for (rlvlno = 0; rlvlno < tcmpt->numrlvls; ++rlvlno) {
		tcmpt->prcwidthexpns[rlvlno] = cp->tccp.prcwidthexpns[rlvlno];
		tcmpt->prcheightexpns[rlvlno] = cp->tccp.prcheightexpns[rlvlno];
	}
	tcmpt->cblkwidthexpn = cp->tccp.cblkwidthexpn;
	tcmpt->cblkheightexpn = cp->tccp.cblkheightexpn;
	tcmpt->cblksty = cp->tccp.cblksty;
	tcmpt->csty = cp->tccp.csty;

	tcmpt->numstepsizes = tcmpt->numbands;
	assert(tcmpt->numstepsizes <= JPC_MAXBANDS);
	memset(tcmpt->stepsizes, 0, tcmpt->numstepsizes * sizeof(uint_fast16_t));

	/* Retrieve information about the various bands. */
	jpc_tsfb_getbands(tcmpt->tsfb, jas_seq2d_xstart(tcmpt->data),
	  jas_seq2d_ystart(tcmpt->data), jas_seq2d_xend(tcmpt->data),
	  jas_seq2d_yend(tcmpt->data), bandinfos);

	if (!(tcmpt->rlvls = jas_malloc(tcmpt->numrlvls * sizeof(jpc_enc_rlvl_t)))) {
		goto error;
	}
	for (rlvlno = 0, rlvl = tcmpt->rlvls; rlvlno < tcmpt->numrlvls;
	  ++rlvlno, ++rlvl) {
		rlvl->bands = 0;
		rlvl->tcmpt = tcmpt;
	}
	for (rlvlno = 0, rlvl = tcmpt->rlvls; rlvlno < tcmpt->numrlvls;
	  ++rlvlno, ++rlvl) {
		if (!rlvl_create(rlvl, cp, tcmpt, bandinfos)) {
			goto error;
		}
	}

	return tcmpt;

error:

	tcmpt_destroy(tcmpt);
	return 0;

}