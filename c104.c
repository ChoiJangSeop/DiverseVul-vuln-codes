static void file_add_remove(struct diff_options *options,
		    int addremove, unsigned mode,
		    const unsigned char *sha1,
		    const char *base, const char *path)
{
	int diff = REV_TREE_DIFFERENT;

	/*
	 * Is it an add of a new file? It means that the old tree
	 * didn't have it at all, so we will turn "REV_TREE_SAME" ->
	 * "REV_TREE_NEW", but leave any "REV_TREE_DIFFERENT" alone
	 * (and if it already was "REV_TREE_NEW", we'll keep it
	 * "REV_TREE_NEW" of course).
	 */
	if (addremove == '+') {
		diff = tree_difference;
		if (diff != REV_TREE_SAME)
			return;
		diff = REV_TREE_NEW;
	}
	tree_difference = diff;
	if (tree_difference == REV_TREE_DIFFERENT)
		DIFF_OPT_SET(options, HAS_CHANGES);
}