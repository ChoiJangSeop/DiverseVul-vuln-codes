static int grep_tree(struct grep_opt *opt, const char **paths,
		     struct tree_desc *tree,
		     const char *tree_name, const char *base)
{
	int len;
	int hit = 0;
	struct name_entry entry;
	char *down;
	int tn_len = strlen(tree_name);
	char *path_buf = xmalloc(PATH_MAX + tn_len + 100);

	if (tn_len) {
		tn_len = sprintf(path_buf, "%s:", tree_name);
		down = path_buf + tn_len;
		strcat(down, base);
	}
	else {
		down = path_buf;
		strcpy(down, base);
	}
	len = strlen(path_buf);

	while (tree_entry(tree, &entry)) {
		strcpy(path_buf + len, entry.path);

		if (S_ISDIR(entry.mode))
			/* Match "abc/" against pathspec to
			 * decide if we want to descend into "abc"
			 * directory.
			 */
			strcpy(path_buf + len + tree_entry_len(entry.path, entry.sha1), "/");

		if (!pathspec_matches(paths, down))
			;
		else if (S_ISREG(entry.mode))
			hit |= grep_sha1(opt, entry.sha1, path_buf, tn_len);
		else if (S_ISDIR(entry.mode)) {
			enum object_type type;
			struct tree_desc sub;
			void *data;
			unsigned long size;

			data = read_sha1_file(entry.sha1, &type, &size);
			if (!data)
				die("unable to read tree (%s)",
				    sha1_to_hex(entry.sha1));
			init_tree_desc(&sub, data, size);
			hit |= grep_tree(opt, paths, &sub, tree_name, down);
			free(data);
		}
	}
	return hit;
}