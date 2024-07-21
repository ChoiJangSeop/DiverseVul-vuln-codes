static int oidc_cache_crypto_decrypt_impl(request_rec *r,
		unsigned char *ciphertext, int ciphertext_len, const unsigned char *aad,
		int aad_len, const unsigned char *tag, int tag_len, unsigned char *key,
		const unsigned char *iv, int iv_len, unsigned char *plaintext) {
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* create and initialize the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_new");
		return -1;
	}

	/* initialize the decryption cipher */
	if (!EVP_DecryptInit_ex(ctx, OIDC_CACHE_CIPHER, NULL, NULL, NULL)) {
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptInit_ex");
		return -1;
	}

	/* set IV length */
	if (!EVP_CIPHER_CTX_ctrl(ctx, OIDC_CACHE_CRYPTO_SET_IVLEN, iv_len, NULL)) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_ctrl");
		return -1;
	}

	/* initialize key and IV */
	if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) {
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptInit_ex");
		return -1;
	}

	/* provide AAD data */
	if (!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len)) {
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptUpdate aad: aad_len=%d",
				aad_len);
		return -1;
	}

	/* provide the message to be decrypted and obtain the plaintext output */
	if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptUpdate ciphertext");
		return -1;
	}
	plaintext_len = len;

	/* set expected tag value; works in OpenSSL 1.0.1d and later */
	if (!EVP_CIPHER_CTX_ctrl(ctx, OIDC_CACHE_CRYPTO_SET_TAG, tag_len,
			(void *) tag)) {
		oidc_cache_crypto_openssl_error(r, "EVP_CIPHER_CTX_ctrl");
		return -1;
	}

	/*
	 * finalize the decryption; a positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* clean up */
	EVP_CIPHER_CTX_free(ctx);

	if (ret > 0) {
		/* success */
		plaintext_len += len;
		return plaintext_len;
	} else {
		/* verify failed */
		oidc_cache_crypto_openssl_error(r, "EVP_DecryptFinal_ex");
		return -1;
	}
}