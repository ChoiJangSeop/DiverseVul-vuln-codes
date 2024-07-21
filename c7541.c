void fslib_copy_libs(const char *full_path) {
	assert(full_path);
	if (arg_debug || arg_debug_private_lib)
		printf("    fslib_copy_libs %s\n", full_path);

	// if library/executable does not exist or the user does not have read access to it
	// print a warning and exit the function.
	if (access(full_path, R_OK)) {
		if (arg_debug || arg_debug_private_lib)
			printf("cannot find %s for private-lib, skipping...\n", full_path);
		return;
	}

	// create an empty RUN_LIB_FILE and allow the user to write to it
	unlink(RUN_LIB_FILE);			  // in case is there
	create_empty_file_as_root(RUN_LIB_FILE, 0644);
	if (chown(RUN_LIB_FILE, getuid(), getgid()))
		errExit("chown");

	// run fldd to extract the list of files
	if (arg_debug || arg_debug_private_lib)
		printf("    running fldd %s\n", full_path);
	sbox_run(SBOX_USER | SBOX_SECCOMP | SBOX_CAPS_NONE, 3, PATH_FLDD, full_path, RUN_LIB_FILE);

	// open the list of libraries and install them on by one
	FILE *fp = fopen(RUN_LIB_FILE, "r");
	if (!fp)
		errExit("fopen");

	char buf[MAXBUF];
	while (fgets(buf, MAXBUF, fp)) {
		// remove \n
		char *ptr = strchr(buf, '\n');
		if (ptr)
			*ptr = '\0';
		fslib_duplicate(buf);
	}
	fclose(fp);
}