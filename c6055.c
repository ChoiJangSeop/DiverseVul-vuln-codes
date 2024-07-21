int crypto_authenticate_and_decrypt (struct crypto_instance *instance,
	unsigned char *buf,
	int *buf_len)
{
	struct crypto_config_header *cch = (struct crypto_config_header *)buf;
	const char *guessed_str;

	if (cch->crypto_cipher_type != CRYPTO_CIPHER_TYPE_2_3) {
		guessed_str = NULL;

		if ((cch->crypto_cipher_type == 0xC0 && cch->crypto_hash_type == 0x70) ||
		    (cch->crypto_cipher_type == 0x70 && cch->crypto_hash_type == 0xC0)) {
			guessed_str = "Corosync 3.x";
		} else if (cch->crypto_cipher_type == CRYPTO_CIPHER_TYPE_2_2) {
			guessed_str = "Corosync 2.2";
		} else if (cch->crypto_cipher_type == 0x01) {
			guessed_str = "unencrypted Kronosnet";
		} else if (cch->crypto_cipher_type >= 0 && cch->crypto_cipher_type <= 5) {
			guessed_str = "unencrypted Corosync 2.0/2.1/1.x/OpenAIS";
		} else {
			guessed_str = "encrypted Kronosnet/Corosync 2.0/2.1/1.x/OpenAIS or unknown";
		}

		log_printf(instance->log_level_security,
		   "Unsupported incoming packet (probably sent by %s). Rejecting",
		   guessed_str);

		return -1;
	}

	if (cch->crypto_hash_type != CRYPTO_HASH_TYPE_2_3) {
		log_printf(instance->log_level_security,
			   "Incoming packet has different hash type. Rejecting");
		return -1;
	}

	/*
	 * authenticate packet first
	 */

	if (authenticate_nss_2_3(instance, buf, buf_len) != 0) {
		return -1;
	}

	/*
	 * now we can "trust" the padding bytes/future features
	 */

	if ((cch->__pad0 != 0) || (cch->__pad1 != 0)) {
		log_printf(instance->log_level_security,
			   "Incoming packet appears to have features not supported by this version of corosync. Rejecting");
		return -1;
	}

	/*
	 * decrypt
	 */

	if (decrypt_nss_2_3(instance, buf, buf_len) != 0) {
		return -1;
	}

	/*
	 * invalidate config header and kill it
	 */
	cch = NULL;
	memmove(buf, buf + sizeof(struct crypto_config_header), *buf_len);

	return 0;
}