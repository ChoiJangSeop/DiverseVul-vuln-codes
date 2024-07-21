TEE_Result syscall_cryp_state_alloc(unsigned long algo, unsigned long mode,
			unsigned long key1, unsigned long key2,
			uint32_t *state)
{
	TEE_Result res;
	struct tee_cryp_state *cs;
	struct tee_ta_session *sess;
	struct tee_obj *o1 = NULL;
	struct tee_obj *o2 = NULL;
	struct user_ta_ctx *utc;

	res = tee_ta_get_current_session(&sess);
	if (res != TEE_SUCCESS)
		return res;
	utc = to_user_ta_ctx(sess->ctx);

	if (key1 != 0) {
		res = tee_obj_get(utc, tee_svc_uref_to_vaddr(key1), &o1);
		if (res != TEE_SUCCESS)
			return res;
		if (o1->busy)
			return TEE_ERROR_BAD_PARAMETERS;
		res = tee_svc_cryp_check_key_type(o1, algo, mode);
		if (res != TEE_SUCCESS)
			return res;
	}
	if (key2 != 0) {
		res = tee_obj_get(utc, tee_svc_uref_to_vaddr(key2), &o2);
		if (res != TEE_SUCCESS)
			return res;
		if (o2->busy)
			return TEE_ERROR_BAD_PARAMETERS;
		res = tee_svc_cryp_check_key_type(o2, algo, mode);
		if (res != TEE_SUCCESS)
			return res;
	}

	cs = calloc(1, sizeof(struct tee_cryp_state));
	if (!cs)
		return TEE_ERROR_OUT_OF_MEMORY;
	TAILQ_INSERT_TAIL(&utc->cryp_states, cs, link);
	cs->algo = algo;
	cs->mode = mode;

	switch (TEE_ALG_GET_CLASS(algo)) {
	case TEE_OPERATION_EXTENSION:
#ifdef CFG_CRYPTO_RSASSA_NA1
		if (algo == TEE_ALG_RSASSA_PKCS1_V1_5)
			goto rsassa_na1;
#endif
		res = TEE_ERROR_NOT_SUPPORTED;
		break;
	case TEE_OPERATION_CIPHER:
		if ((algo == TEE_ALG_AES_XTS && (key1 == 0 || key2 == 0)) ||
		    (algo != TEE_ALG_AES_XTS && (key1 == 0 || key2 != 0))) {
			res = TEE_ERROR_BAD_PARAMETERS;
		} else {
			res = crypto_cipher_alloc_ctx(&cs->ctx, algo);
			if (res != TEE_SUCCESS)
				break;
		}
		break;
	case TEE_OPERATION_AE:
		if (key1 == 0 || key2 != 0) {
			res = TEE_ERROR_BAD_PARAMETERS;
		} else {
			res = crypto_authenc_alloc_ctx(&cs->ctx, algo);
			if (res != TEE_SUCCESS)
				break;
		}
		break;
	case TEE_OPERATION_MAC:
		if (key1 == 0 || key2 != 0) {
			res = TEE_ERROR_BAD_PARAMETERS;
		} else {
			res = crypto_mac_alloc_ctx(&cs->ctx, algo);
			if (res != TEE_SUCCESS)
				break;
		}
		break;
	case TEE_OPERATION_DIGEST:
		if (key1 != 0 || key2 != 0) {
			res = TEE_ERROR_BAD_PARAMETERS;
		} else {
			res = crypto_hash_alloc_ctx(&cs->ctx, algo);
			if (res != TEE_SUCCESS)
				break;
		}
		break;
	case TEE_OPERATION_ASYMMETRIC_CIPHER:
	case TEE_OPERATION_ASYMMETRIC_SIGNATURE:
rsassa_na1: __maybe_unused
		if (key1 == 0 || key2 != 0)
			res = TEE_ERROR_BAD_PARAMETERS;
		break;
	case TEE_OPERATION_KEY_DERIVATION:
		if (key1 == 0 || key2 != 0)
			res = TEE_ERROR_BAD_PARAMETERS;
		break;
	default:
		res = TEE_ERROR_NOT_SUPPORTED;
		break;
	}
	if (res != TEE_SUCCESS)
		goto out;

	res = tee_svc_copy_kaddr_to_uref(state, cs);
	if (res != TEE_SUCCESS)
		goto out;

	/* Register keys */
	if (o1 != NULL) {
		o1->busy = true;
		cs->key1 = (vaddr_t)o1;
	}
	if (o2 != NULL) {
		o2->busy = true;
		cs->key2 = (vaddr_t)o2;
	}

out:
	if (res != TEE_SUCCESS)
		cryp_state_free(utc, cs);
	return res;
}