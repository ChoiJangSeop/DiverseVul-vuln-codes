vlog_message(const int facility, const char* format, va_list args)
{
#if !HAVE_VSYSLOG
	char buf[MAX_LOG_MSG+1];

	vsnprintf(buf, sizeof(buf), format, args);
#endif

	/* Don't write syslog if testing configuration */
	if (__test_bit(CONFIG_TEST_BIT, &debug))
		return;

	if (log_file || (__test_bit(DONT_FORK_BIT, &debug) && log_console)) {
#if HAVE_VSYSLOG
		va_list args1;
		char buf[2 * MAX_LOG_MSG + 1];

		va_copy(args1, args);
		vsnprintf(buf, sizeof(buf), format, args1);
		va_end(args1);
#endif

		/* timestamp setup */
		time_t t = time(NULL);
		struct tm tm;
		localtime_r(&t, &tm);
		char timestamp[64];
		strftime(timestamp, sizeof(timestamp), "%c", &tm);

		if (log_console && __test_bit(DONT_FORK_BIT, &debug))
			fprintf(stderr, "%s: %s\n", timestamp, buf);
		if (log_file) {
			fprintf(log_file, "%s: %s\n", timestamp, buf);
			if (always_flush_log_file)
				fflush(log_file);
		}
	}

	if (!__test_bit(NO_SYSLOG_BIT, &debug))
#if HAVE_VSYSLOG
		vsyslog(facility, format, args);
#else
		syslog(facility, "%s", buf);
#endif
}