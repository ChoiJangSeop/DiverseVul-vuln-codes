static int store_xauthority(void) {
	// put a copy of .Xauthority in XAUTHORITY_FILE
	fs_build_mnt_dir();

	char *src;
	char *dest = RUN_XAUTHORITY_FILE;
	// create an empty file 
	FILE *fp = fopen(dest, "w");
	if (fp) {
		fprintf(fp, "\n");
		SET_PERMS_STREAM(fp, getuid(), getgid(), 0600);
		fclose(fp);
	}

	if (asprintf(&src, "%s/.Xauthority", cfg.homedir) == -1)
		errExit("asprintf");
	
	struct stat s;
	if (stat(src, &s) == 0) {
		if (is_link(src)) {
			fprintf(stderr, "Error: invalid .Xauthority file\n");
			exit(1);
		}
			
		pid_t child = fork();
		if (child < 0)
			errExit("fork");
		if (child == 0) {
			// drop privileges
			drop_privs(0);
	
			// copy, set permissions and ownership
			int rv = copy_file(src, dest);
			if (rv)
				fprintf(stderr, "Warning: cannot transfer .Xauthority in private home directory\n");
			else {
				fs_logger2("clone", dest);
			}
				
			_exit(0);
		}
		// wait for the child to finish
		waitpid(child, NULL, 0);

		if (chown(dest, getuid(), getgid()) == -1)
			errExit("fchown");
		if (chmod(dest, 0600) == -1)
			errExit("fchmod");
		return 1; // file copied
	}
	
	return 0;
}