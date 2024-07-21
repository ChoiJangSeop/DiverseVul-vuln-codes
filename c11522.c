int find_child(pid_t parent, pid_t *child) {
	EUID_ASSERT();
	*child = 0;				  // use it to flag a found child

	DIR *dir;
	EUID_ROOT();				  // grsecurity fix
	if (!(dir = opendir("/proc"))) {
		// sleep 2 seconds and try again
		sleep(2);
		if (!(dir = opendir("/proc"))) {
			fprintf(stderr, "Error: cannot open /proc directory\n");
			exit(1);
		}
	}

	struct dirent *entry;
	char *end;
	while (*child == 0 && (entry = readdir(dir))) {
		pid_t pid = strtol(entry->d_name, &end, 10);
		if (end == entry->d_name || *end)
			continue;
		if (pid == parent)
			continue;

		// open stat file
		char *file;
		if (asprintf(&file, "/proc/%u/status", pid) == -1) {
			perror("asprintf");
			exit(1);
		}
		FILE *fp = fopen(file, "re");
		if (!fp) {
			free(file);
			continue;
		}

		// look for firejail executable name
		char buf[BUFLEN];
		while (fgets(buf, BUFLEN - 1, fp)) {
			if (strncmp(buf, "PPid:", 5) == 0) {
				char *ptr = buf + 5;
				while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t')) {
					ptr++;
				}
				if (*ptr == '\0') {
					fprintf(stderr, "Error: cannot read /proc file\n");
					exit(1);
				}
				if (parent == atoi(ptr)) {
					// we don't want /usr/bin/xdg-dbus-proxy!
					char *cmdline = pid_proc_cmdline(pid);
					if (cmdline) {
						if (strncmp(cmdline, XDG_DBUS_PROXY_PATH, strlen(XDG_DBUS_PROXY_PATH)) != 0)
							*child = pid;
						free(cmdline);
					}
				}
				break;		  // stop reading the file
			}
		}
		fclose(fp);
		free(file);
	}
	closedir(dir);
	EUID_USER();
	return (*child)? 0:1;			  // 0 = found, 1 = not found
}