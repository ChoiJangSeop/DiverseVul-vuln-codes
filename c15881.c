TEE_Result syscall_hash_update(unsigned long state, const void *chunk,
			size_t chunk_size)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;

	/* No data, but size provided isn't valid parameters. */
	if (!chunk && chunk_size)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Zero length hash is valid, but nothing we need to do. */
	if (!chunk_size)
		return TEE_SUCCESS;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t)chunk, chunk_size);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	switch (TEE_ALG_GET_CLASS(cs->algo)) {
	case TEE_OPERATION_DIGEST:
		res = crypto_hash_update(cs->ctx, cs->algo, chunk, chunk_size);
		if (res != TEE_SUCCESS)
			return res;
		break;
	case TEE_OPERATION_MAC:
		res = crypto_mac_update(cs->ctx, cs->algo, chunk, chunk_size);
		if (res != TEE_SUCCESS)
			return res;
		break;
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;
}