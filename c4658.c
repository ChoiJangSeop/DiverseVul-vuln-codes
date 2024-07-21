static jpc_enc_cblk_t *cblk_create(jpc_enc_cblk_t *cblk, jpc_enc_cp_t *cp, jpc_enc_prc_t *prc)
{
	jpc_enc_band_t *band;
	uint_fast32_t cblktlx;
	uint_fast32_t cblktly;
	uint_fast32_t cblkbrx;
	uint_fast32_t cblkbry;
	jpc_enc_rlvl_t *rlvl;
	uint_fast32_t cblkxind;
	uint_fast32_t cblkyind;
	uint_fast32_t cblkno;
	uint_fast32_t tlcblktlx;
	uint_fast32_t tlcblktly;

	cblkno = cblk - prc->cblks;
	cblkxind = cblkno % prc->numhcblks;
	cblkyind = cblkno / prc->numhcblks;
	rlvl = prc->band->rlvl;
	cblk->prc = prc;

	cblk->numpasses = 0;
	cblk->passes = 0;
	cblk->numencpasses = 0;
	cblk->numimsbs = 0;
	cblk->numlenbits = 0;
	cblk->stream = 0;
	cblk->mqenc = 0;
	cblk->flags = 0;
	cblk->numbps = 0;
	cblk->curpass = 0;
	cblk->data = 0;
	cblk->savedcurpass = 0;
	cblk->savednumlenbits = 0;
	cblk->savednumencpasses = 0;

	band = prc->band;
	tlcblktlx = JPC_FLOORTOMULTPOW2(prc->tlx, rlvl->cblkwidthexpn);
	tlcblktly = JPC_FLOORTOMULTPOW2(prc->tly, rlvl->cblkheightexpn);
	cblktlx = JAS_MAX(tlcblktlx + (cblkxind << rlvl->cblkwidthexpn), prc->tlx);
	cblktly = JAS_MAX(tlcblktly + (cblkyind << rlvl->cblkheightexpn), prc->tly);
	cblkbrx = JAS_MIN(tlcblktlx + ((cblkxind + 1) << rlvl->cblkwidthexpn),
	  prc->brx);
	cblkbry = JAS_MIN(tlcblktly + ((cblkyind + 1) << rlvl->cblkheightexpn),
	  prc->bry);

	assert(cblktlx < cblkbrx && cblktly < cblkbry);
	if (!(cblk->data = jas_seq2d_create(0, 0, 0, 0))) {
		goto error;
	}
	jas_seq2d_bindsub(cblk->data, band->data, cblktlx, cblktly, cblkbrx, cblkbry);

	return cblk;

error:
	cblk_destroy(cblk);
	return 0;
}