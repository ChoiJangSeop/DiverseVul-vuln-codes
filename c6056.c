static int authenticate_nss_2_3 (
	struct crypto_instance *instance,
	unsigned char *buf,
	int *buf_len)
{
	if (hash_to_nss[instance->crypto_hash_type]) {
		unsigned char	tmp_hash[hash_len[instance->crypto_hash_type]];
		int             datalen = *buf_len - hash_len[instance->crypto_hash_type];

		if (calculate_nss_hash(instance, buf, datalen, tmp_hash) < 0) {
			return -1;
		}

		if (memcmp(tmp_hash, buf + datalen, hash_len[instance->crypto_hash_type]) != 0) {
			log_printf(instance->log_level_error, "Digest does not match");
			return -1;
		}
		*buf_len = datalen;
	}

	return 0;
}