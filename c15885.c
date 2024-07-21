TEE_Result syscall_authenc_update_aad(unsigned long state,
			const void *aad_data, size_t aad_data_len)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t) aad_data,
					  aad_data_len);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	if (TEE_ALG_GET_CLASS(cs->algo) != TEE_OPERATION_AE)
		return TEE_ERROR_BAD_STATE;

	res = crypto_authenc_update_aad(cs->ctx, cs->algo, cs->mode,
					aad_data, aad_data_len);
	if (res != TEE_SUCCESS)
		return res;

	return TEE_SUCCESS;
}