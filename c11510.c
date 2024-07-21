void create_empty_file_as_root(const char *fname, mode_t mode) {
	assert(fname);
	mode &= 07777;
	struct stat s;

	if (stat(fname, &s)) {
		if (arg_debug)
			printf("Creating empty %s file\n", fname);
		/* coverity[toctou] */
		// don't fail if file already exists. This can be the case in a race
		// condition, when two jails launch at the same time. Compare to #1013
		FILE *fp = fopen(fname, "we");
		if (!fp)
			errExit("fopen");
		SET_PERMS_STREAM(fp, 0, 0, mode);
		fclose(fp);
	}
}