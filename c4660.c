static void tcmpt_destroy(jpc_enc_tcmpt_t *tcmpt)
{
	jpc_enc_rlvl_t *rlvl;
	uint_fast16_t rlvlno;

	if (tcmpt->rlvls) {
		for (rlvlno = 0, rlvl = tcmpt->rlvls; rlvlno < tcmpt->numrlvls;
		  ++rlvlno, ++rlvl) {
			rlvl_destroy(rlvl);
		}
		jas_free(tcmpt->rlvls);
	}

	if (tcmpt->data) {
		jas_seq2d_destroy(tcmpt->data);
	}
	if (tcmpt->tsfb) {
		jpc_tsfb_destroy(tcmpt->tsfb);
	}
}