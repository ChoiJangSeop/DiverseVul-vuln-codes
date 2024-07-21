apr_byte_t oidc_cache_get(request_rec *r, const char *section, const char *key,
		char **value) {

	oidc_cfg *cfg = ap_get_module_config(r->server->module_config,
			&auth_openidc_module);
	int encrypted = oidc_cfg_cache_encrypt(r);
	apr_byte_t rc = TRUE;
	char *msg = NULL;

	oidc_debug(r, "enter: %s (section=%s, decrypt=%d, type=%s)", key, section,
			encrypted, cfg->cache->name);

	/* see if encryption is turned on */
	if (encrypted == 1)
		key = oidc_cache_get_hashed_key(r, cfg->crypto_passphrase, key);

	/* get the value from the cache */
	const char *cache_value = NULL;
	if (cfg->cache->get(r, section, key, &cache_value) == FALSE) {
		rc = FALSE;
		goto out;
	}

	/* see if it is any good */
	if (cache_value == NULL)
		goto out;

	/* see if encryption is turned on */
	if (encrypted == 0) {
		*value = apr_pstrdup(r->pool, cache_value);
		goto out;
	}

	rc = (oidc_cache_crypto_decrypt(r, cache_value,
			oidc_cache_hash_passphrase(r, cfg->crypto_passphrase),
			(unsigned char **) value) > 0);

out:
	/* log the result */
	msg = apr_psprintf(r->pool, "from %s cache backend for %skey %s",
			cfg->cache->name, encrypted ? "encrypted " : "", key);
	if (rc == TRUE)
		if (*value != NULL)
			oidc_debug(r, "cache hit: return %d bytes %s",
					*value ? (int )strlen(*value) : 0, msg);
		else
			oidc_debug(r, "cache miss %s", msg);
	else
		oidc_warn(r, "error retrieving value %s", msg);

	return rc;
}