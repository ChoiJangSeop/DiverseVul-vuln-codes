static void show_entry(struct diff_options *opt, const char *prefix, struct tree_desc *desc,
		       const char *base, int baselen)
{
	unsigned mode;
	const char *path;
	const unsigned char *sha1 = tree_entry_extract(desc, &path, &mode);

	if (DIFF_OPT_TST(opt, RECURSIVE) && S_ISDIR(mode)) {
		enum object_type type;
		int pathlen = tree_entry_len(path, sha1);
		char *newbase = malloc_base(base, baselen, path, pathlen);
		struct tree_desc inner;
		void *tree;
		unsigned long size;

		tree = read_sha1_file(sha1, &type, &size);
		if (!tree || type != OBJ_TREE)
			die("corrupt tree sha %s", sha1_to_hex(sha1));

		init_tree_desc(&inner, tree, size);
		show_tree(opt, prefix, &inner, newbase, baselen + 1 + pathlen);

		free(tree);
		free(newbase);
	} else {
		opt->add_remove(opt, prefix[0], mode, sha1, base, path);
	}
}