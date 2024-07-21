TEE_Result syscall_cipher_init(unsigned long state, const void *iv,
			size_t iv_len)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;
	struct tee_obj *o;
	struct tee_cryp_obj_secret *key1;
	struct user_ta_ctx *utc;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;
	utc = to_user_ta_ctx(sess->ctx);

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	if (TEE_ALG_GET_CLASS(cs->algo) != TEE_OPERATION_CIPHER)
		return TEE_ERROR_BAD_STATE;

	res = tee_mmu_check_access_rights(utc,
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t) iv, iv_len);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_obj_get(utc, cs->key1, &o);
	if (res != TEE_SUCCESS)
		return res;
	if ((o->info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	key1 = o->attr;

	if (tee_obj_get(utc, cs->key2, &o) == TEE_SUCCESS) {
		struct tee_cryp_obj_secret *key2 = o->attr;

		if ((o->info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
			return TEE_ERROR_BAD_PARAMETERS;

		res = crypto_cipher_init(cs->ctx, cs->algo, cs->mode,
					 (uint8_t *)(key1 + 1), key1->key_size,
					 (uint8_t *)(key2 + 1), key2->key_size,
					 iv, iv_len);
	} else {
		res = crypto_cipher_init(cs->ctx, cs->algo, cs->mode,
					 (uint8_t *)(key1 + 1), key1->key_size,
					 NULL, 0, iv, iv_len);
	}
	if (res != TEE_SUCCESS)
		return res;

	cs->ctx_finalize = crypto_cipher_final;
	return TEE_SUCCESS;
}