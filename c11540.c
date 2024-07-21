void check_join_permission(pid_t pid) {
	// check if pid belongs to a fully set up firejail sandbox
	unsigned long i;
	for (i = SNOOZE; is_ready_for_join(pid) == false; i += SNOOZE) { // give sandbox some time to start up
		if (i > join_timeout) {
			fprintf(stderr, "Error: no valid sandbox\n");
			exit(1);
		}
		usleep(SNOOZE);
	}
	// check privileges for non-root users
	uid_t uid = getuid();
	if (uid != 0) {
		uid_t sandbox_uid = pid_get_uid(pid);
		if (uid != sandbox_uid) {
			fprintf(stderr, "Error: permission is denied to join a sandbox created by a different user.\n");
			exit(1);
		}
	}
}