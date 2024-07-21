static void band_destroy(jpc_enc_band_t *band)
{
	jpc_enc_prc_t *prc;
	jpc_enc_rlvl_t *rlvl;
	uint_fast32_t prcno;

	if (band->prcs) {
		rlvl = band->rlvl;
		for (prcno = 0, prc = band->prcs; prcno < rlvl->numprcs;
		  ++prcno, ++prc) {
			prc_destroy(prc);
		}
		jas_free(band->prcs);
	}
	if (band->data) {
		jas_seq2d_destroy(band->data);
	}
}