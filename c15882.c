TEE_Result syscall_hash_final(unsigned long state, const void *chunk,
			size_t chunk_size, void *hash, uint64_t *hash_len)
{
	TEE_Result res, res2;
	size_t hash_size;
	size_t hlen = 0;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;

	/* No data, but size provided isn't valid parameters. */
	if (!chunk && chunk_size)
		return TEE_ERROR_BAD_PARAMETERS;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t)chunk, chunk_size);
	if (res != TEE_SUCCESS)
		return res;

	res = get_user_u64_as_size_t(&hlen, hash_len);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_mmu_check_access_rights(to_user_ta_ctx(sess->ctx),
					  TEE_MEMORY_ACCESS_READ |
					  TEE_MEMORY_ACCESS_WRITE |
					  TEE_MEMORY_ACCESS_ANY_OWNER,
					  (uaddr_t)hash, hlen);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	switch (TEE_ALG_GET_CLASS(cs->algo)) {
	case TEE_OPERATION_DIGEST:
		res = tee_hash_get_digest_size(cs->algo, &hash_size);
		if (res != TEE_SUCCESS)
			return res;
		if (hlen < hash_size) {
			res = TEE_ERROR_SHORT_BUFFER;
			goto out;
		}

		if (chunk_size) {
			res = crypto_hash_update(cs->ctx, cs->algo, chunk,
						 chunk_size);
			if (res != TEE_SUCCESS)
				return res;
		}

		res = crypto_hash_final(cs->ctx, cs->algo, hash, hash_size);
		if (res != TEE_SUCCESS)
			return res;
		break;

	case TEE_OPERATION_MAC:
		res = tee_mac_get_digest_size(cs->algo, &hash_size);
		if (res != TEE_SUCCESS)
			return res;
		if (hlen < hash_size) {
			res = TEE_ERROR_SHORT_BUFFER;
			goto out;
		}

		if (chunk_size) {
			res = crypto_mac_update(cs->ctx, cs->algo, chunk,
						chunk_size);
			if (res != TEE_SUCCESS)
				return res;
		}

		res = crypto_mac_final(cs->ctx, cs->algo, hash, hash_size);
		if (res != TEE_SUCCESS)
			return res;
		break;

	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
out:
	res2 = put_user_u64(hash_len, hash_size);
	if (res2 != TEE_SUCCESS)
		return res2;
	return res;
}