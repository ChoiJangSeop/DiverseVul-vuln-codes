static inline int r_sys_mkdirp(char *dir) {
	int ret = 1;
	const char slash = DIRSEP;
	char *path = dir;
	char *ptr = path;
	if (*ptr == slash) {
		ptr++;
	}
#if __SDB_WINDOWS__
	char *p = strstr (ptr, ":\\");
	if (p) {
		ptr = p + 2;
	}
#endif
	while ((ptr = strchr (ptr, slash))) {
		*ptr = 0;
		if (!r_sys_mkdir (path) && r_sys_mkdir_failed ()) {
			eprintf ("r_sys_mkdirp: fail '%s' of '%s'\n", path, dir);
			*ptr = slash;
			return 0;
		}
		*ptr = slash;
		ptr++;
	}
	return ret;
}