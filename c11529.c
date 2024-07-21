static void extract_user_namespace(pid_t pid) {
	// test user namespaces available in the kernel
	struct stat s1;
	struct stat s2;
	struct stat s3;
	if (stat("/proc/self/ns/user", &s1) == 0 &&
	    stat("/proc/self/uid_map", &s2) == 0 &&
	    stat("/proc/self/gid_map", &s3) == 0);
	else
		return;

	// read uid map
	char *uidmap;
	if (asprintf(&uidmap, "/proc/%u/uid_map", pid) == -1)
		errExit("asprintf");
	FILE *fp = fopen(uidmap, "re");
	if (!fp) {
		free(uidmap);
		return;
	}

	// check uid map
	int u1;
	int u2;
	if (fscanf(fp, "%d %d", &u1, &u2) == 2) {
		if (arg_debug)
			printf("User namespace detected: %s, %d, %d\n", uidmap, u1, u2);
		if (u1 != 0 || u2 != 0)
			arg_noroot = 1;
	}
	fclose(fp);
	free(uidmap);
}