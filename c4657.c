void jpc_enc_tile_destroy(jpc_enc_tile_t *tile)
{
	jpc_enc_tcmpt_t *tcmpt;
	uint_fast16_t cmptno;

	if (tile->tcmpts) {
		for (cmptno = 0, tcmpt = tile->tcmpts; cmptno <
		  tile->numtcmpts; ++cmptno, ++tcmpt) {
			tcmpt_destroy(tcmpt);
		}
		jas_free(tile->tcmpts);
	}
	if (tile->lyrsizes) {
		jas_free(tile->lyrsizes);
	}
	if (tile->pi) {
		jpc_pi_destroy(tile->pi);
	}
	jas_free(tile);
}