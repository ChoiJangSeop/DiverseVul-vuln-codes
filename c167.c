static jpc_enc_band_t *band_create(jpc_enc_band_t *band, jpc_enc_cp_t *cp,
  jpc_enc_rlvl_t *rlvl, jpc_tsfb_band_t *bandinfos)
{
	uint_fast16_t bandno;
	uint_fast16_t gblbandno;
	uint_fast16_t rlvlno;
	jpc_tsfb_band_t *bandinfo;
	jpc_enc_tcmpt_t *tcmpt;
	uint_fast32_t prcno;
	jpc_enc_prc_t *prc;

	tcmpt = rlvl->tcmpt;
	band->data = 0;
	band->prcs = 0;
	band->rlvl = rlvl;

	/* Deduce the resolution level and band number. */
	rlvlno = rlvl - rlvl->tcmpt->rlvls;
	bandno = band - rlvl->bands;
	gblbandno = (!rlvlno) ? 0 : (3 * (rlvlno - 1) + bandno + 1);

	bandinfo = &bandinfos[gblbandno];

if (bandinfo->xstart != bandinfo->xend && bandinfo->ystart != bandinfo->yend) {
	if (!(band->data = jas_seq2d_create(0, 0, 0, 0))) {
		goto error;
	}
	jas_seq2d_bindsub(band->data, tcmpt->data, bandinfo->locxstart,
	  bandinfo->locystart, bandinfo->locxend, bandinfo->locyend);
	jas_seq2d_setshift(band->data, bandinfo->xstart, bandinfo->ystart);
}
	band->orient = bandinfo->orient;
	band->analgain = JPC_NOMINALGAIN(cp->tccp.qmfbid, tcmpt->numrlvls, rlvlno,
	  band->orient);
	band->numbps = 0;
	band->absstepsize = 0;
	band->stepsize = 0;
	band->synweight = bandinfo->synenergywt;

if (band->data) {
	if (!(band->prcs = jas_malloc(rlvl->numprcs * sizeof(jpc_enc_prc_t)))) {
		goto error;
	}
	for (prcno = 0, prc = band->prcs; prcno < rlvl->numprcs; ++prcno,
	  ++prc) {
		prc->cblks = 0;
		prc->incltree = 0;
		prc->nlibtree = 0;
		prc->savincltree = 0;
		prc->savnlibtree = 0;
		prc->band = band;
	}
	for (prcno = 0, prc = band->prcs; prcno < rlvl->numprcs; ++prcno,
	  ++prc) {
		if (!prc_create(prc, cp, band)) {
			goto error;
		}
	}
}

	return band;

error:
	band_destroy(band);
	return 0;
}