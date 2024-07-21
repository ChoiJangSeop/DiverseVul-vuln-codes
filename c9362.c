pkcs11rsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	CK_RV rv;
	pk11_context_t *pk11_ctx = dctx->ctxdata.pk11_ctx;
	isc_result_t ret = ISC_R_SUCCESS;

	PK11_CALL(pkcs_C_VerifyFinal,
		  (pk11_ctx->session,
		   (CK_BYTE_PTR) sig->base,
		   (CK_ULONG) sig->length),
		  DST_R_VERIFYFAILURE);
	return (ret);
}