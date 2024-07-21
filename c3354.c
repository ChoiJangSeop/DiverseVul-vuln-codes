authtrust(
	keyid_t		id,
	u_long		trust
	)
{
	symkey **	bucket;
	symkey *	sk;
	u_long		lifetime;

	/*
	 * Search bin for key; if it does not exist and is untrusted,
	 * forget it.
	 */
	bucket = &key_hash[KEYHASH(id)];
	for (sk = *bucket; sk != NULL; sk = sk->hlink) {
		if (id == sk->keyid)
			break;
	}
	if (!trust && NULL == sk)
		return;

	/*
	 * There are two conditions remaining. Either it does not
	 * exist and is to be trusted or it does exist and is or is
	 * not to be trusted.
	 */	
	if (sk != NULL) {
		if (cache_keyid == id) {
			cache_flags = 0;
			cache_keyid = 0;
		}

		/*
		 * Key exists. If it is to be trusted, say so and
		 * update its lifetime. 
		 */
		if (trust > 0) {
			sk->flags |= KEY_TRUSTED;
			if (trust > 1)
				sk->lifetime = current_time + trust;
			else
				sk->lifetime = 0;
			return;
		}

		/* No longer trusted, return it to the free list. */
		freesymkey(sk, bucket);
		return;
	}

	/*
	 * keyid is not present, but the is to be trusted.  We allocate
	 * a new key, but do not specify a key type or secret.
	 */
	if (trust > 1) {
		lifetime = current_time + trust;
	} else {
		lifetime = 0;
	}
	allocsymkey(bucket, id, KEY_TRUSTED, 0, lifetime, 0, NULL);
}