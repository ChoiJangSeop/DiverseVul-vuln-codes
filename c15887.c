TEE_Result syscall_authenc_init(unsigned long state, const void *nonce,
			size_t nonce_len, size_t tag_len,
			size_t aad_len, size_t payload_len)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;
	struct tee_obj *o;
	struct tee_cryp_obj_secret *key;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t)nonce, nonce_len);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_obj_get(to_user_ta_ctx(sess->ctx), cs->key1, &o);
	if (res != TEE_SUCCESS)
		return res;
	if ((o->info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	key = o->attr;
	res = crypto_authenc_init(cs->ctx, cs->algo, cs->mode,
				  (uint8_t *)(key + 1), key->key_size,
				  nonce, nonce_len, tag_len, aad_len,
				  payload_len);
	if (res != TEE_SUCCESS)
		return res;

	cs->ctx_finalize = (tee_cryp_ctx_finalize_func_t)crypto_authenc_final;
	return TEE_SUCCESS;
}