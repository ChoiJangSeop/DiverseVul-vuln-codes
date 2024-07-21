static void file_add_remove(struct diff_options *options,
		    int addremove, unsigned mode,
		    const unsigned char *sha1,
		    int sha1_valid,
		    const char *fullpath, unsigned dirty_submodule)
{
	int diff = addremove == '+' ? REV_TREE_NEW : REV_TREE_OLD;

	tree_difference |= diff;
	if (tree_difference == REV_TREE_DIFFERENT)
		DIFF_OPT_SET(options, HAS_CHANGES);
}