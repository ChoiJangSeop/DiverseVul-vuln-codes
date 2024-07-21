static int threaded_has_dirs_only_path(struct cache_def *cache, const char *name, int len, int prefix_len)
{
	return lstat_cache(cache, name, len,
			   FL_DIR|FL_FULLPATH, prefix_len) &
		FL_DIR;
}