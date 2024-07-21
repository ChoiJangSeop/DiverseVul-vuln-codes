unsigned int clean_fname(char *name, int flags)
{
	char *limit = name - 1, *t = name, *f = name;
	int anchored;

	if (!name)
		return 0;

	if ((anchored = *f == '/') != 0) {
		*t++ = *f++;
#ifdef __CYGWIN__
		/* If there are exactly 2 slashes at the start, preserve
		 * them.  Would break daemon excludes unless the paths are
		 * really treated differently, so used this sparingly. */
		if (*f == '/' && f[1] != '/')
			*t++ = *f++;
#endif
	} else if (flags & CFN_KEEP_DOT_DIRS && *f == '.' && f[1] == '/') {
		*t++ = *f++;
		*t++ = *f++;
	}
	while (*f) {
		/* discard extra slashes */
		if (*f == '/') {
			f++;
			continue;
		}
		if (*f == '.') {
			/* discard interior "." dirs */
			if (f[1] == '/' && !(flags & CFN_KEEP_DOT_DIRS)) {
				f += 2;
				continue;
			}
			if (f[1] == '\0' && flags & CFN_DROP_TRAILING_DOT_DIR)
				break;
			/* collapse ".." dirs */
			if (flags & CFN_COLLAPSE_DOT_DOT_DIRS
			 && f[1] == '.' && (f[2] == '/' || !f[2])) {
				char *s = t - 1;
				if (s == name && anchored) {
					f += 2;
					continue;
				}
				while (s > limit && *--s != '/') {}
				if (s != t - 1 && (s < name || *s == '/')) {
					t = s + 1;
					f += 2;
					continue;
				}
				limit = t + 2;
			}
		}
		while (*f && (*t++ = *f++) != '/') {}
	}

	if (t > name+anchored && t[-1] == '/' && !(flags & CFN_KEEP_TRAILING_SLASH))
		t--;
	if (t == name)
		*t++ = '.';
	*t = '\0';

	return t - name;
}