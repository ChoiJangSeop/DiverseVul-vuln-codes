static int jpc_dec_tilefini(jpc_dec_t *dec, jpc_dec_tile_t *tile)
{
	jpc_dec_tcomp_t *tcomp;
	int compno;
	int bandno;
	int rlvlno;
	jpc_dec_band_t *band;
	jpc_dec_rlvl_t *rlvl;
	int prcno;
	jpc_dec_prc_t *prc;
	jpc_dec_seg_t *seg;
	jpc_dec_cblk_t *cblk;
	int cblkno;

	if (tile->tcomps) {

		for (compno = 0, tcomp = tile->tcomps; compno < dec->numcomps;
		  ++compno, ++tcomp) {
			for (rlvlno = 0, rlvl = tcomp->rlvls; rlvlno < tcomp->numrlvls;
			  ++rlvlno, ++rlvl) {
				if (!rlvl->bands) {
					continue;
				}
				for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
				  ++bandno, ++band) {
					if (band->prcs) {
						for (prcno = 0, prc = band->prcs; prcno <
						  rlvl->numprcs; ++prcno, ++prc) {
							if (!prc->cblks) {
								continue;
							}
							for (cblkno = 0, cblk = prc->cblks; cblkno <
							  prc->numcblks; ++cblkno, ++cblk) {

								while (cblk->segs.head) {
									seg = cblk->segs.head;
									jpc_seglist_remove(&cblk->segs, seg);
									jpc_seg_destroy(seg);
								}
								jas_matrix_destroy(cblk->data);
								if (cblk->mqdec) {
									jpc_mqdec_destroy(cblk->mqdec);
								}
								if (cblk->nulldec) {
									jpc_bitstream_close(cblk->nulldec);
								}
								if (cblk->flags) {
									jas_matrix_destroy(cblk->flags);
								}
							}
							if (prc->incltagtree) {
								jpc_tagtree_destroy(prc->incltagtree);
							}
							if (prc->numimsbstagtree) {
								jpc_tagtree_destroy(prc->numimsbstagtree);
							}
							if (prc->cblks) {
								jas_free(prc->cblks);
							}
						}
					}
					if (band->data) {
						jas_matrix_destroy(band->data);
					}
					if (band->prcs) {
						jas_free(band->prcs);
					}
				}
				if (rlvl->bands) {
					jas_free(rlvl->bands);
				}
			}
			if (tcomp->rlvls) {
				jas_free(tcomp->rlvls);
			}
			if (tcomp->data) {
				jas_matrix_destroy(tcomp->data);
			}
			if (tcomp->tsfb) {
				jpc_tsfb_destroy(tcomp->tsfb);
			}
		}
	}

	if (tile->cp) {
		jpc_dec_cp_destroy(tile->cp);
		tile->cp = 0;
	}
	if (tile->tcomps) {
		jas_free(tile->tcomps);
		tile->tcomps = 0;
	}
	if (tile->pi) {
		jpc_pi_destroy(tile->pi);
		tile->pi = 0;
	}
	if (tile->pkthdrstream) {
		jas_stream_close(tile->pkthdrstream);
		tile->pkthdrstream = 0;
	}
	if (tile->pptstab) {
		jpc_ppxstab_destroy(tile->pptstab);
		tile->pptstab = 0;
	}

	tile->state = JPC_TILE_DONE;

	return 0;
}