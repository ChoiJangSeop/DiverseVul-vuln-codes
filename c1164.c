static void totem_get_crypto(struct totem_config *totem_config)
{
	char *str;
	const char *tmp_cipher;
	const char *tmp_hash;

	tmp_hash = "sha1";
	tmp_cipher = "aes256";

	if (icmap_get_string("totem.secauth", &str) == CS_OK) {
		if (strcmp (str, "off") == 0) {
			tmp_hash = "none";
			tmp_cipher = "none";
		}
		free(str);
	}

	if (icmap_get_string("totem.crypto_cipher", &str) == CS_OK) {
		if (strcmp(str, "none") == 0) {
			tmp_cipher = "none";
		}
		if (strcmp(str, "aes256") == 0) {
			tmp_cipher = "aes256";
		}
		if (strcmp(str, "aes192") == 0) {
			tmp_cipher = "aes192";
		}
		if (strcmp(str, "aes128") == 0) {
			tmp_cipher = "aes128";
		}
		if (strcmp(str, "3des") == 0) {
			tmp_cipher = "3des";
		}
		free(str);
	}

	if (icmap_get_string("totem.crypto_hash", &str) == CS_OK) {
		if (strcmp(str, "none") == 0) {
			tmp_hash = "none";
		}
		if (strcmp(str, "md5") == 0) {
			tmp_hash = "md5";
		}
		if (strcmp(str, "sha1") == 0) {
			tmp_hash = "sha1";
		}
		if (strcmp(str, "sha256") == 0) {
			tmp_hash = "sha256";
		}
		if (strcmp(str, "sha384") == 0) {
			tmp_hash = "sha384";
		}
		if (strcmp(str, "sha512") == 0) {
			tmp_hash = "sha512";
		}
		free(str);
	}

	free(totem_config->crypto_cipher_type);
	free(totem_config->crypto_hash_type);

	totem_config->crypto_cipher_type = strdup(tmp_cipher);
	totem_config->crypto_hash_type = strdup(tmp_hash);
}