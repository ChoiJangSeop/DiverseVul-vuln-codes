ext4_xattr_create_cache(char *name)
{
	return mb_cache_create(name, HASH_BUCKET_BITS);
}