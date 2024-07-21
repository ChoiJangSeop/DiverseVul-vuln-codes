apr_byte_t oidc_cache_set(request_rec *r, const char *section, const char *key,
		const char *value, apr_time_t expiry) {

	oidc_cfg *cfg = ap_get_module_config(r->server->module_config,
			&auth_openidc_module);
	int encrypted = oidc_cfg_cache_encrypt(r);
	char *encoded = NULL;
	apr_byte_t rc = FALSE;
	char *msg = NULL;

	oidc_debug(r,
			"enter: %s (section=%s, len=%d, encrypt=%d, ttl(s)=%" APR_TIME_T_FMT ", type=%s)",
			key, section, value ? (int )strlen(value) : 0, encrypted,
					apr_time_sec(expiry - apr_time_now()), cfg->cache->name);

	/* see if we need to encrypt */
	if (encrypted == 1) {

		key = oidc_cache_get_hashed_key(r, cfg->crypto_passphrase, key);
		if (key == NULL)
			goto out;

		if (value != NULL) {
			if (oidc_cache_crypto_encrypt(r, value,
					oidc_cache_hash_passphrase(r, cfg->crypto_passphrase),
					&encoded) <= 0)
				goto out;
			value = encoded;
		}
	}

	/* store the resulting value in the cache */
	rc = cfg->cache->set(r, section, key, value, expiry);

out:
	/* log the result */
	msg = apr_psprintf(r->pool, "%d bytes in %s cache backend for %skey %s",
			(value ? (int) strlen(value) : 0),
			(cfg->cache->name ? cfg->cache->name : ""),
			(encrypted ? "encrypted " : ""), (key ? key : ""));
	if (rc == TRUE)
		oidc_debug(r, "successfully stored %s", msg);
	else
		oidc_warn(r, "could NOT store %s", msg);

	return rc;
}