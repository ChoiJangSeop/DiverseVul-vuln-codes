void shut(pid_t pid) {
	EUID_ASSERT();

	EUID_ROOT();
	char *comm = pid_proc_comm(pid);
	EUID_USER();
	if (comm) {
		if (strcmp(comm, "firejail") != 0) {
			fprintf(stderr, "Error: this is not a firejail sandbox\n");
			exit(1);
		}
		free(comm);
	}
	else {
		fprintf(stderr, "Error: cannot find process %d\n", pid);
		exit(1);
	}

	// check privileges for non-root users
	uid_t uid = getuid();
	if (uid != 0) {
		uid_t sandbox_uid = pid_get_uid(pid);
		if (uid != sandbox_uid) {
			fprintf(stderr, "Error: permission is denied to shutdown a sandbox created by a different user.\n");
			exit(1);
		}
	}

	printf("Sending SIGTERM to %u\n", pid);
	kill(pid, SIGTERM);

	// wait for not more than 11 seconds
	int monsec = 11;
	char *monfile;
	if (asprintf(&monfile, "/proc/%d/cmdline", pid) == -1)
		errExit("asprintf");
	int killdone = 0;

	while (monsec) {
		sleep(1);
		monsec--;

		EUID_ROOT();
		FILE *fp = fopen(monfile, "re");
		EUID_USER();
		if (!fp) {
			killdone = 1;
			break;
		}

		char c;
		size_t count = fread(&c, 1, 1, fp);
		fclose(fp);
		if (count == 0) {
			// all done
			killdone = 1;
			break;
		}
	}
	free(monfile);


	// force SIGKILL
	if (!killdone) {
		// kill the process and its child
		pid_t child;
		if (find_child(pid, &child) == 0) {
			printf("Sending SIGKILL to %u\n", child);
			kill(child, SIGKILL);
		}
		printf("Sending SIGKILL to %u\n", pid);
		kill(pid, SIGKILL);
	}

	EUID_ROOT();
	delete_run_files(pid);
}