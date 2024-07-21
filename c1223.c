static char *build_alias(char *alias, const char *device_name)
{
	char *dest;

	/* ">bar/": rename to bar/device_name */
	/* ">bar[/]baz": rename to bar[/]baz */
	dest = strrchr(alias, '/');
	if (dest) { /* ">bar/[baz]" ? */
		*dest = '\0'; /* mkdir bar */
		bb_make_directory(alias, 0755, FILEUTILS_RECUR);
		*dest = '/';
		if (dest[1] == '\0') { /* ">bar/" => ">bar/device_name" */
			dest = alias;
			alias = concat_path_file(alias, device_name);
			free(dest);
		}
	}

	return alias;
}