u32 flow_hash_from_keys(struct flow_keys *keys)
{
	__flow_hash_secret_init();
	return __flow_hash_from_keys(keys, hashrnd);
}