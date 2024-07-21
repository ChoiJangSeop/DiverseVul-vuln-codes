TEE_Result syscall_authenc_update_payload(unsigned long state,
			const void *src_data, size_t src_len, void *dst_data,
			uint64_t *dst_len)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;
	size_t dlen = 0;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	if (TEE_ALG_GET_CLASS(cs->algo) != TEE_OPERATION_AE)
		return TEE_ERROR_BAD_STATE;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t) src_data, src_len);
	if (res != TEE_SUCCESS)
		return res;

	res = get_user_u64_as_size_t(&dlen, dst_len);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_WRITE |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t)dst_data, dlen);
	if (res != TEE_SUCCESS)
		return res;

	if (dlen < src_len) {
		res = TEE_ERROR_SHORT_BUFFER;
		goto out;
	}

	res = crypto_authenc_update_payload(cs->ctx, cs->algo, cs->mode,
					    src_data, src_len, dst_data,
					    &dlen);
out:
	if (res == TEE_SUCCESS || res == TEE_ERROR_SHORT_BUFFER) {
		TEE_Result res2 = put_user_u64(dst_len, dlen);

		if (res2 != TEE_SUCCESS)
			res = res2;
	}

	return res;
}