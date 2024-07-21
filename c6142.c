int jpc_enc_enccblks(jpc_enc_t *enc)
{
	jpc_enc_tcmpt_t *tcmpt;
	jpc_enc_tcmpt_t *endcomps;
	jpc_enc_rlvl_t *lvl;
	jpc_enc_rlvl_t *endlvls;
	jpc_enc_band_t *band;
	jpc_enc_band_t *endbands;
	jpc_enc_cblk_t *cblk;
	jpc_enc_cblk_t *endcblks;
	int i;
	int j;
	jpc_fix_t mx;
	jpc_fix_t bmx;
	jpc_fix_t v;
	jpc_enc_tile_t *tile;
	uint_fast32_t prcno;
	jpc_enc_prc_t *prc;

	tile = enc->curtile;

	endcomps = &tile->tcmpts[tile->numtcmpts];
	for (tcmpt = tile->tcmpts; tcmpt != endcomps; ++tcmpt) {
		endlvls = &tcmpt->rlvls[tcmpt->numrlvls];
		for (lvl = tcmpt->rlvls; lvl != endlvls; ++lvl) {
			if (!lvl->bands) {
				continue;
			}
			endbands = &lvl->bands[lvl->numbands];
			for (band = lvl->bands; band != endbands; ++band) {
				if (!band->data) {
					continue;
				}
				for (prcno = 0, prc = band->prcs; prcno < lvl->numprcs; ++prcno, ++prc) {
					if (!prc->cblks) {
						continue;
					}
					bmx = 0;
					endcblks = &prc->cblks[prc->numcblks];
					for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
						mx = 0;
						for (i = 0; i < jas_matrix_numrows(cblk->data); ++i) {
							for (j = 0; j < jas_matrix_numcols(cblk->data); ++j) {
								v = JAS_ABS(jas_matrix_get(cblk->data, i, j));
								if (v > mx) {
									mx = v;
								}
							}
						}
						if (mx > bmx) {
							bmx = mx;
						}
						cblk->numbps = JAS_MAX(jpc_firstone(mx) + 1 - JPC_NUMEXTRABITS, 0);
					}

					for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
						cblk->numimsbs = band->numbps - cblk->numbps;
						assert(cblk->numimsbs >= 0);
					}

					for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
						if (jpc_enc_enccblk(enc, cblk->stream, tcmpt, band, cblk)) {
							return -1;
						}
					}
				}
			}
		}
	}
	return 0;
}