void wait_for_other(int fd) {
	//****************************
	// wait for the parent to be initialized
	//****************************
	char childstr[BUFLEN + 1];
	int newfd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
	if (newfd == -1)
		errExit("fcntl");
	FILE* stream;
	stream = fdopen(newfd, "r");
	*childstr = '\0';
	if (fgets(childstr, BUFLEN, stream)) {
		// remove \n)
		char *ptr = childstr;
		while(*ptr !='\0' && *ptr != '\n')
			ptr++;
		if (*ptr == '\0')
			errExit("fgets");
		*ptr = '\0';
	}
	else {
		fprintf(stderr, "Error: proc %d cannot sync with peer: %s\n",
			getpid(), ferror(stream) ? strerror(errno) : "unexpected EOF");

		int status = 0;
		pid_t pid = wait(&status);
		if (pid != -1) {
			if (WIFEXITED(status))
				fprintf(stderr, "Peer %d unexpectedly exited with status %d\n",
					pid, WEXITSTATUS(status));
			else if (WIFSIGNALED(status))
				fprintf(stderr, "Peer %d unexpectedly killed (%s)\n",
					pid, strsignal(WTERMSIG(status)));
			else
				fprintf(stderr, "Peer %d unexpectedly exited "
					"(un-decodable wait status %04x)\n", pid, status);
		}
		exit(1);
	}

	if (strcmp(childstr, "arg_noroot=0") == 0)
		arg_noroot = 0;
	else if (strcmp(childstr, "arg_noroot=1") == 0)
		arg_noroot = 1;
	else {
		fprintf(stderr, "Error: unexpected message from peer: %s\n", childstr);
		exit(1);
	}

	fclose(stream);
}