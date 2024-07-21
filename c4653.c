static jpc_enc_prc_t *prc_create(jpc_enc_prc_t *prc, jpc_enc_cp_t *cp, jpc_enc_band_t *band)
{
	uint_fast32_t prcno;
	uint_fast32_t prcxind;
	uint_fast32_t prcyind;
	uint_fast32_t cbgtlx;
	uint_fast32_t cbgtly;
	uint_fast32_t tlprctlx;
	uint_fast32_t tlprctly;
	uint_fast32_t tlcbgtlx;
	uint_fast32_t tlcbgtly;
	uint_fast16_t rlvlno;
	jpc_enc_rlvl_t *rlvl;
	uint_fast32_t tlcblktlx;
	uint_fast32_t tlcblktly;
	uint_fast32_t brcblkbrx;
	uint_fast32_t brcblkbry;
	uint_fast32_t cblkno;
	jpc_enc_cblk_t *cblk;
	jpc_enc_tcmpt_t *tcmpt;

	prc->cblks = 0;
	prc->incltree = 0;
	prc->savincltree = 0;
	prc->nlibtree = 0;
	prc->savnlibtree = 0;

	rlvl = band->rlvl;
	tcmpt = rlvl->tcmpt;
rlvlno = rlvl - tcmpt->rlvls;
	prcno = prc - band->prcs;
	prcxind = prcno % rlvl->numhprcs;
	prcyind = prcno / rlvl->numhprcs;
	prc->band = band;

tlprctlx = JPC_FLOORTOMULTPOW2(rlvl->tlx, rlvl->prcwidthexpn);
tlprctly = JPC_FLOORTOMULTPOW2(rlvl->tly, rlvl->prcheightexpn);
if (!rlvlno) {
	tlcbgtlx = tlprctlx;
	tlcbgtly = tlprctly;
} else {
	tlcbgtlx = JPC_CEILDIVPOW2(tlprctlx, 1);
	tlcbgtly = JPC_CEILDIVPOW2(tlprctly, 1);
}

	/* Compute the coordinates of the top-left and bottom-right
	  corners of the precinct. */
	cbgtlx = tlcbgtlx + (prcxind << rlvl->cbgwidthexpn);
	cbgtly = tlcbgtly + (prcyind << rlvl->cbgheightexpn);
	prc->tlx = JAS_MAX(jas_seq2d_xstart(band->data), cbgtlx);
	prc->tly = JAS_MAX(jas_seq2d_ystart(band->data), cbgtly);
	prc->brx = JAS_MIN(jas_seq2d_xend(band->data), cbgtlx +
	  (1 << rlvl->cbgwidthexpn));
	prc->bry = JAS_MIN(jas_seq2d_yend(band->data), cbgtly +
	  (1 << rlvl->cbgheightexpn));

	if (prc->tlx < prc->brx && prc->tly < prc->bry) {
		/* The precinct contains at least one code block. */

		tlcblktlx = JPC_FLOORTOMULTPOW2(prc->tlx, rlvl->cblkwidthexpn);
		tlcblktly = JPC_FLOORTOMULTPOW2(prc->tly, rlvl->cblkheightexpn);
		brcblkbrx = JPC_CEILTOMULTPOW2(prc->brx, rlvl->cblkwidthexpn);
		brcblkbry = JPC_CEILTOMULTPOW2(prc->bry, rlvl->cblkheightexpn);
		prc->numhcblks = JPC_FLOORDIVPOW2(brcblkbrx - tlcblktlx,
		  rlvl->cblkwidthexpn);
		prc->numvcblks = JPC_FLOORDIVPOW2(brcblkbry - tlcblktly,
		  rlvl->cblkheightexpn);
		prc->numcblks = prc->numhcblks * prc->numvcblks;

		if (!(prc->incltree = jpc_tagtree_create(prc->numhcblks,
		  prc->numvcblks))) {
			goto error;
		}
		if (!(prc->nlibtree = jpc_tagtree_create(prc->numhcblks,
		  prc->numvcblks))) {
			goto error;
		}
		if (!(prc->savincltree = jpc_tagtree_create(prc->numhcblks,
		  prc->numvcblks))) {
			goto error;
		}
		if (!(prc->savnlibtree = jpc_tagtree_create(prc->numhcblks,
		  prc->numvcblks))) {
			goto error;
		}

		if (!(prc->cblks = jas_alloc2(prc->numcblks, sizeof(jpc_enc_cblk_t)))) {
			goto error;
		}
		for (cblkno = 0, cblk = prc->cblks; cblkno < prc->numcblks;
		  ++cblkno, ++cblk) {
			cblk->passes = 0;
			cblk->stream = 0;
			cblk->mqenc = 0;
			cblk->data = 0;
			cblk->flags = 0;
			cblk->prc = prc;
		}
		for (cblkno = 0, cblk = prc->cblks; cblkno < prc->numcblks;
		  ++cblkno, ++cblk) {
			if (!cblk_create(cblk, cp, prc)) {
				goto error;
			}
		}
	} else {
		/* The precinct does not contain any code blocks. */
		prc->tlx = prc->brx;
		prc->tly = prc->bry;
		prc->numcblks = 0;
		prc->numhcblks = 0;
		prc->numvcblks = 0;
		prc->cblks = 0;
		prc->incltree = 0;
		prc->nlibtree = 0;
		prc->savincltree = 0;
		prc->savnlibtree = 0;
	}

	return prc;

error:
	prc_destroy(prc);
	return 0;
}