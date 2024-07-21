transform_cipher_param(struct rte_crypto_sym_xform *xform,
		VhostUserCryptoSessionParam *param)
{
	int ret;

	ret = cipher_algo_transform(param->cipher_algo, &xform->cipher.algo);
	if (unlikely(ret < 0))
		return ret;

	xform->type = RTE_CRYPTO_SYM_XFORM_CIPHER;
	xform->cipher.key.length = param->cipher_key_len;
	if (xform->cipher.key.length > 0)
		xform->cipher.key.data = param->cipher_key_buf;
	if (param->dir == VIRTIO_CRYPTO_OP_ENCRYPT)
		xform->cipher.op = RTE_CRYPTO_CIPHER_OP_ENCRYPT;
	else if (param->dir == VIRTIO_CRYPTO_OP_DECRYPT)
		xform->cipher.op = RTE_CRYPTO_CIPHER_OP_DECRYPT;
	else {
		VC_LOG_DBG("Bad operation type");
		return -VIRTIO_CRYPTO_BADMSG;
	}

	ret = get_iv_len(xform->cipher.algo);
	if (unlikely(ret < 0))
		return ret;
	xform->cipher.iv.length = (uint16_t)ret;
	xform->cipher.iv.offset = IV_OFFSET;
	return 0;
}