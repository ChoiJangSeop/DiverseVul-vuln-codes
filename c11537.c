void enter_network_namespace(pid_t pid) {
	// in case the pid is that of a firejail process, use the pid of the first child process
	pid_t child = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(child);

	// check network namespace
	char *name;
	if (asprintf(&name, "/run/firejail/network/%d-netmap", pid) == -1)
		errExit("asprintf");
	struct stat s;
	if (stat(name, &s) == -1) {
		fprintf(stderr, "Error: the sandbox doesn't use a new network namespace\n");
		exit(1);
	}

	// join the namespace
	EUID_ROOT();
	if (join_namespace(child, "net")) {
		fprintf(stderr, "Error: cannot join the network namespace\n");
		exit(1);
	}
}