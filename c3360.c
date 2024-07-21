authhavekey(
	keyid_t		id
	)
{
	symkey *	sk;

	authkeylookups++;
	if (0 == id || cache_keyid == id) {
		return TRUE;
	}

	/*
	 * Seach the bin for the key. If found and the key type
	 * is zero, somebody marked it trusted without specifying
	 * a key or key type. In this case consider the key missing.
	 */
	authkeyuncached++;
	for (sk = key_hash[KEYHASH(id)]; sk != NULL; sk = sk->hlink) {
		if (id == sk->keyid) {
			if (0 == sk->type) {
				authkeynotfound++;
				return FALSE;
			}
			break;
		}
	}

	/*
	 * If the key is not found, or if it is found but not trusted,
	 * the key is not considered found.
	 */
	if (NULL == sk) {
		authkeynotfound++;
		return FALSE;
	}
	if (!(KEY_TRUSTED & sk->flags)) {
		authnokey++;
		return FALSE;
	}

	/*
	 * The key is found and trusted. Initialize the key cache.
	 */
	cache_keyid = sk->keyid;
	cache_type = sk->type;
	cache_flags = sk->flags;
	cache_secret = sk->secret;
	cache_secretsize = sk->secretsize;

	return TRUE;
}