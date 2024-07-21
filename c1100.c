void set_cron_watched(int fd) {
	pid_t pid = getpid();
	int i;

	if (fd < 0) {
		inotify_enabled = 0;
		return;
	}

	for (i = 0; i < sizeof (wd) / sizeof (wd[0]); ++i) {
		int w;

		if (open(watchpaths[i], O_RDONLY | O_NONBLOCK, 0) != -1) {
			w = inotify_add_watch(fd, watchpaths[i],
				IN_CREATE | IN_CLOSE_WRITE | IN_ATTRIB | IN_MODIFY | IN_MOVED_TO |
				IN_MOVED_FROM | IN_MOVE_SELF | IN_DELETE | IN_DELETE_SELF);
			if (w < 0) {
				if (wd[i] != -1) {
					log_it("CRON", pid, "This directory or file can't be watched",
						watchpaths[i], errno);
					log_it("CRON", pid, "INFO", "running without inotify support", 0);
				}
				inotify_enabled = 0;
				set_cron_unwatched(fd);
				return;
			}
			wd[i] = w;
		}
	}

	if (!inotify_enabled) {
		log_it("CRON", pid, "INFO", "running with inotify support", 0);
	}

	inotify_enabled = 1;
}