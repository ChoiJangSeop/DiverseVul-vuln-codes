TEE_Result syscall_cryp_state_copy(unsigned long dst, unsigned long src)
{
	TEE_Result res;
	struct tee_cryp_state *cs_dst;
	struct tee_cryp_state *cs_src;
	struct tee_ta_session *sess;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(dst), &cs_dst);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(src), &cs_src);
	if (res != TEE_SUCCESS)
		return res;
	if (cs_dst->algo != cs_src->algo || cs_dst->mode != cs_src->mode)
		return TEE_ERROR_BAD_PARAMETERS;

	switch (TEE_ALG_GET_CLASS(cs_src->algo)) {
	case TEE_OPERATION_CIPHER:
		crypto_cipher_copy_state(cs_dst->ctx, cs_src->ctx,
					 cs_src->algo);
		break;
	case TEE_OPERATION_AE:
		crypto_authenc_copy_state(cs_dst->ctx, cs_src->ctx,
					  cs_src->algo);
		break;
	case TEE_OPERATION_DIGEST:
		crypto_hash_copy_state(cs_dst->ctx, cs_src->ctx, cs_src->algo);
		break;
	case TEE_OPERATION_MAC:
		crypto_mac_copy_state(cs_dst->ctx, cs_src->ctx, cs_src->algo);
		break;
	default:
		return TEE_ERROR_BAD_STATE;
	}

	return TEE_SUCCESS;
}