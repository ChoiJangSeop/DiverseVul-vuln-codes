static int oidc_cache_crypto_encrypt_impl(request_rec *r,
		unsigned char *plaintext, int plaintext_len, const unsigned char *aad,
		int aad_len, unsigned char *key, const unsigned char *iv, int iv_len,
		unsigned char *ciphertext, const unsigned char *tag, int tag_len) {
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;

	/* create and initialize the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_new");
		return -1;
	}

	/* initialize the encryption cipher */
	if (!EVP_EncryptInit_ex(ctx, OIDC_CACHE_CIPHER, NULL, NULL, NULL)) {
		oidc_cache_crypto_openssl_error(r, "EVP_EncryptInit_ex");
		return -1;
	}

	/* set IV length */
	if (!EVP_CIPHER_CTX_ctrl(ctx, OIDC_CACHE_CRYPTO_SET_IVLEN, iv_len, NULL)) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_ctrl");
		return -1;
	}

	/* initialize key and IV */
	if (!EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) {
		oidc_cache_crypto_openssl_error(r, "EVP_EncryptInit_ex");
		return -1;
	}

	/* provide AAD data */
	if (!EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len)) {
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptUpdate aad: aad_len=%d",
				aad_len);
		return -1;
	}

	/* provide the message to be encrypted and obtain the encrypted output */
	if (!EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
		oidc_cache_crypto_openssl_error(r, "EVP_EncryptUpdate ciphertext");
		return -1;
	}
	ciphertext_len = len;

	/*
	 * finalize the encryption; normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if (!EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
		oidc_cache_crypto_openssl_error(r, "EVP_EncryptFinal_ex");
		return -1;
	}
	ciphertext_len += len;

	/* get the tag */
	if (!EVP_CIPHER_CTX_ctrl(ctx, OIDC_CACHE_CRYPTO_GET_TAG, tag_len,
			(void *) tag)) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_ctrl");
		return -1;
	}

	/* clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}