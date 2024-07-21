static void prepare_attr_stack(const char *path, int dirlen)
{
	struct attr_stack *elem, *info;
	int len;
	char pathbuf[PATH_MAX];

	/*
	 * At the bottom of the attribute stack is the built-in
	 * set of attribute definitions.  Then, contents from
	 * .gitattribute files from directories closer to the
	 * root to the ones in deeper directories are pushed
	 * to the stack.  Finally, at the very top of the stack
	 * we always keep the contents of $GIT_DIR/info/attributes.
	 *
	 * When checking, we use entries from near the top of the
	 * stack, preferring $GIT_DIR/info/attributes, then
	 * .gitattributes in deeper directories to shallower ones,
	 * and finally use the built-in set as the default.
	 */
	if (!attr_stack)
		bootstrap_attr_stack();

	/*
	 * Pop the "info" one that is always at the top of the stack.
	 */
	info = attr_stack;
	attr_stack = info->prev;

	/*
	 * Pop the ones from directories that are not the prefix of
	 * the path we are checking.
	 */
	while (attr_stack && attr_stack->origin) {
		int namelen = strlen(attr_stack->origin);

		elem = attr_stack;
		if (namelen <= dirlen &&
		    !strncmp(elem->origin, path, namelen))
			break;

		debug_pop(elem);
		attr_stack = elem->prev;
		free_attr_elem(elem);
	}

	/*
	 * Read from parent directories and push them down
	 */
	if (!is_bare_repository()) {
		while (1) {
			char *cp;

			len = strlen(attr_stack->origin);
			if (dirlen <= len)
				break;
			memcpy(pathbuf, path, dirlen);
			memcpy(pathbuf + dirlen, "/", 2);
			cp = strchr(pathbuf + len + 1, '/');
			strcpy(cp + 1, GITATTRIBUTES_FILE);
			elem = read_attr(pathbuf, 0);
			*cp = '\0';
			elem->origin = strdup(pathbuf);
			elem->prev = attr_stack;
			attr_stack = elem;
			debug_push(elem);
		}
	}

	/*
	 * Finally push the "info" one at the top of the stack.
	 */
	info->prev = attr_stack;
	attr_stack = info;
}