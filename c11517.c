void caps_print_filter(pid_t pid) {
	EUID_ASSERT();

	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	uint64_t caps = extract_caps(pid);
	int i;
	uint64_t mask;
	int elems = sizeof(capslist) / sizeof(capslist[0]);
	for (i = 0, mask = 1; i < elems; i++, mask <<= 1) {
		printf("%-18.18s  - %s\n", capslist[i].name, (mask & caps)? "enabled": "disabled");
	}

	exit(0);
}