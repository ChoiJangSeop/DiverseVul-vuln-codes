void create_empty_dir_as_root(const char *dir, mode_t mode) {
	assert(dir);
	mode &= 07777;
	struct stat s;

	if (stat(dir, &s)) {
		if (arg_debug)
			printf("Creating empty %s directory\n", dir);
		/* coverity[toctou] */
		// don't fail if directory already exists. This can be the case in a race
		// condition, when two jails launch at the same time. See #1013
		if (mkdir(dir, mode) == -1 && errno != EEXIST)
			errExit("mkdir");
		if (set_perms(dir, 0, 0, mode))
			errExit("set_perms");
		ASSERT_PERMS(dir, 0, 0, mode);
	}
}