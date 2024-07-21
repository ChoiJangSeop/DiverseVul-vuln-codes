int has_handler(pid_t pid, int signal) {
	if (signal > 0 && signal <= SIGRTMAX) {
		char *fname;
		if (asprintf(&fname, "/proc/%d/status", pid) == -1)
			errExit("asprintf");
		EUID_ROOT();
		FILE *fp = fopen(fname, "re");
		EUID_USER();
		free(fname);
		if (fp) {
			char buf[BUFLEN];
			while (fgets(buf, BUFLEN, fp)) {
				if (strncmp(buf, "SigCgt:", 7) == 0) {
					unsigned long long val;
					if (sscanf(buf + 7, "%llx", &val) != 1) {
						fprintf(stderr, "Error: cannot read /proc file\n");
						exit(1);
					}
					val >>= (signal - 1);
					val &= 1ULL;
					fclose(fp);
					return val;  // 1 if process has a handler for the signal, else 0
				}
			}
			fclose(fp);
		}
	}
	return 0;
}