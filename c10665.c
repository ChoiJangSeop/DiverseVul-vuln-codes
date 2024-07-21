static int oidc_cache_crypto_decrypt(request_rec *r, const char *cache_value,
		unsigned char *key, unsigned char **plaintext) {

	int len = -1;

	/* grab the base64url-encoded tag after the "." */
	char *encoded_tag = strstr(cache_value, ".");
	if (encoded_tag == NULL) {
		oidc_error(r,
				"corrupted cache value: no tag separator found in encrypted value");
		return FALSE;
	}

	/* make sure we don't modify the original string since it may be just a pointer into the cache (shm) */
	cache_value = apr_pstrmemdup(r->pool, cache_value,
			strlen(cache_value) - strlen(encoded_tag));
	encoded_tag++;

	/* base64url decode the ciphertext */
	char *d_bytes = NULL;
	int d_len = oidc_base64url_decode(r->pool, &d_bytes, cache_value);

	/* base64url decode the tag */
	char *t_bytes = NULL;
	int t_len = oidc_base64url_decode(r->pool, &t_bytes, encoded_tag);

	/* see if we're still good to go */
	if ((d_len > 0) && (t_len > 0)) {

		/* allocated space for the plaintext */
		*plaintext = apr_pcalloc(r->pool,
				(d_len + EVP_CIPHER_block_size(OIDC_CACHE_CIPHER) - 1));

		/* decrypt the ciphertext providing the tag value */

		len = oidc_cache_crypto_decrypt_impl(r, (unsigned char *) d_bytes,
				d_len, OIDC_CACHE_CRYPTO_GCM_AAD,
				sizeof(OIDC_CACHE_CRYPTO_GCM_AAD), (unsigned char *) t_bytes,
				t_len, key, OIDC_CACHE_CRYPTO_GCM_IV,
				sizeof(OIDC_CACHE_CRYPTO_GCM_IV), *plaintext);

		/* check the result and make sure it is \0 terminated */
		if (len > -1) {
			(*plaintext)[len] = '\0';
		} else {
			*plaintext = NULL;
		}

	}

	return len;
}