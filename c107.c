static void diff_index_show_file(struct rev_info *revs,
				 const char *prefix,
				 struct cache_entry *ce,
				 const unsigned char *sha1, unsigned int mode)
{
	diff_addremove(&revs->diffopt, prefix[0], mode,
		       sha1, ce->name, NULL);
}