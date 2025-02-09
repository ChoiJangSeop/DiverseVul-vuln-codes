int user_update(struct key *key, struct key_preparsed_payload *prep)
{
	struct user_key_payload *zap = NULL;
	int ret;

	/* check the quota and attach the new data */
	ret = key_payload_reserve(key, prep->datalen);
	if (ret < 0)
		return ret;

	/* attach the new data, displacing the old */
	key->expiry = prep->expiry;
	if (!test_bit(KEY_FLAG_NEGATIVE, &key->flags))
		zap = dereference_key_locked(key);
	rcu_assign_keypointer(key, prep->payload.data[0]);
	prep->payload.data[0] = NULL;

	if (zap)
		call_rcu(&zap->rcu, user_free_payload_rcu);
	return ret;
}