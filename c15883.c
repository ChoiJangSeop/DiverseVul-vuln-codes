TEE_Result syscall_hash_init(unsigned long state,
			     const void *iv __maybe_unused,
			     size_t iv_len __maybe_unused)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;

	res = tee_svc_cryp_get_state(sess, tee_svc_uref_to_vaddr(state), &cs);
	if (res != TEE_SUCCESS)
		return res;

	switch (TEE_ALG_GET_CLASS(cs->algo)) {
	case TEE_OPERATION_DIGEST:
		res = crypto_hash_init(cs->ctx, cs->algo);
		if (res != TEE_SUCCESS)
			return res;
		break;
	case TEE_OPERATION_MAC:
		{
			struct tee_obj *o;
			struct tee_cryp_obj_secret *key;

			res = tee_obj_get(to_user_ta_ctx(sess->ctx),
					  cs->key1, &o);
			if (res != TEE_SUCCESS)
				return res;
			if ((o->info.handleFlags &
			     TEE_HANDLE_FLAG_INITIALIZED) == 0)
				return TEE_ERROR_BAD_PARAMETERS;

			key = (struct tee_cryp_obj_secret *)o->attr;
			res = crypto_mac_init(cs->ctx, cs->algo,
					      (void *)(key + 1), key->key_size);
			if (res != TEE_SUCCESS)
				return res;
			break;
		}
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;
}