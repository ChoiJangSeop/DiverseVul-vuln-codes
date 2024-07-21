void disable_config(void) {
	EUID_USER();
#ifndef HAVE_ONLY_SYSCFG_PROFILES
	char *fname;
	if (asprintf(&fname, "%s/.config/firejail", cfg.homedir) == -1)
		errExit("asprintf");
	disable_file(BLACKLIST_FILE, fname);
	free(fname);
#endif

	// disable run time information
	disable_file(BLACKLIST_FILE, RUN_FIREJAIL_NETWORK_DIR);
	disable_file(BLACKLIST_FILE, RUN_FIREJAIL_BANDWIDTH_DIR);
	disable_file(BLACKLIST_FILE, RUN_FIREJAIL_NAME_DIR);
	disable_file(BLACKLIST_FILE, RUN_FIREJAIL_PROFILE_DIR);
	disable_file(BLACKLIST_FILE, RUN_FIREJAIL_X11_DIR);
	EUID_ROOT();
}