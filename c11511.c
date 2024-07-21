void cpu_print_filter(pid_t pid) {
	EUID_ASSERT();

	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// now check if the pid belongs to a firejail sandbox
	if (is_ready_for_join(pid) == false) {
		fprintf(stderr, "Error: no valid sandbox\n");
		exit(1);
	}

	print_cpu(pid);
	exit(0);
}