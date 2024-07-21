void net_dns_print(pid_t pid) {
	EUID_ASSERT();
	// drop privileges - will not be able to read /etc/resolv.conf for --noroot option

	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	EUID_ROOT();
	if (join_namespace(pid, "mnt"))
		exit(1);

	pid_t child = fork();
	if (child < 0)
		errExit("fork");
	if (child == 0) {
		caps_drop_all();
		if (chdir("/") < 0)
			errExit("chdir");

		// access /etc/resolv.conf
		FILE *fp = fopen("/etc/resolv.conf", "re");
		if (!fp) {
			fprintf(stderr, "Error: cannot access /etc/resolv.conf\n");
			exit(1);
		}

		char buf[MAXBUF];
		while (fgets(buf, MAXBUF, fp))
			printf("%s", buf);
		printf("\n");
		fclose(fp);
		exit(0);
	}

	// wait for the child to finish
	waitpid(child, NULL, 0);
	flush_stdin();
	exit(0);
}