transform_chain_param(struct rte_crypto_sym_xform *xforms,
		VhostUserCryptoSessionParam *param)
{
	struct rte_crypto_sym_xform *xform_cipher, *xform_auth;
	int ret;

	switch (param->chaining_dir) {
	case VIRTIO_CRYPTO_SYM_ALG_CHAIN_ORDER_HASH_THEN_CIPHER:
		xform_auth = xforms;
		xform_cipher = xforms->next;
		xform_cipher->cipher.op = RTE_CRYPTO_CIPHER_OP_DECRYPT;
		xform_auth->auth.op = RTE_CRYPTO_AUTH_OP_VERIFY;
		break;
	case VIRTIO_CRYPTO_SYM_ALG_CHAIN_ORDER_CIPHER_THEN_HASH:
		xform_cipher = xforms;
		xform_auth = xforms->next;
		xform_cipher->cipher.op = RTE_CRYPTO_CIPHER_OP_ENCRYPT;
		xform_auth->auth.op = RTE_CRYPTO_AUTH_OP_GENERATE;
		break;
	default:
		return -VIRTIO_CRYPTO_BADMSG;
	}

	/* cipher */
	ret = cipher_algo_transform(param->cipher_algo,
			&xform_cipher->cipher.algo);
	if (unlikely(ret < 0))
		return ret;
	xform_cipher->type = RTE_CRYPTO_SYM_XFORM_CIPHER;
	xform_cipher->cipher.key.length = param->cipher_key_len;
	xform_cipher->cipher.key.data = param->cipher_key_buf;
	ret = get_iv_len(xform_cipher->cipher.algo);
	if (unlikely(ret < 0))
		return ret;
	xform_cipher->cipher.iv.length = (uint16_t)ret;
	xform_cipher->cipher.iv.offset = IV_OFFSET;

	/* auth */
	xform_auth->type = RTE_CRYPTO_SYM_XFORM_AUTH;
	ret = auth_algo_transform(param->hash_algo, &xform_auth->auth.algo);
	if (unlikely(ret < 0))
		return ret;
	xform_auth->auth.digest_length = param->digest_len;
	xform_auth->auth.key.length = param->auth_key_len;
	xform_auth->auth.key.data = param->auth_key_buf;

	return 0;
}