static void cblk_destroy(jpc_enc_cblk_t *cblk)
{
	uint_fast16_t passno;
	jpc_enc_pass_t *pass;
	if (cblk->passes) {
		for (passno = 0, pass = cblk->passes; passno < cblk->numpasses;
		  ++passno, ++pass) {
			pass_destroy(pass);
		}
		jas_free(cblk->passes);
	}
	if (cblk->stream) {
		jas_stream_close(cblk->stream);
	}
	if (cblk->mqenc) {
		jpc_mqenc_destroy(cblk->mqenc);
	}
	if (cblk->data) {
		jas_seq2d_destroy(cblk->data);
	}
	if (cblk->flags) {
		jas_seq2d_destroy(cblk->flags);
	}
}