static int qh_core(int sd, char *buf, unsigned int len)
{
	char *space;

	if (*buf == 0 || !strcmp(buf, "help")) {

		nsock_printf_nul(sd, 
			"Query handler for manipulating nagios core.\n"
			"Available commands:\n"
			"  loadctl           Print information about current load control settings\n"
			"  loadctl <options> Configure nagios load control.\n"
			"                    The options are the same parameters and format as\n"
			"                    returned above.\n"
			"  squeuestats       scheduling queue statistics\n"
		);

		return 0;
	}
	space = memchr(buf, ' ', len);

	if (space != NULL) {
		*(space++) = 0;
	}

	if (space == NULL) {

		if (!strcmp(buf, "loadctl")) {

			nsock_printf_nul(sd, 
				"jobs_max=%u;jobs_min=%u;"
				"jobs_running=%u;jobs_limit=%u;"
				"load=%.2f;"
				"backoff_limit=%.2f;backoff_change=%u;"
				"rampup_limit=%.2f;rampup_change=%u;"
				"nproc_limit=%u;nofile_limit=%u;"
				"options=%u;changes=%u;",
				loadctl.jobs_max, loadctl.jobs_min,
				loadctl.jobs_running, loadctl.jobs_limit,
				loadctl.load[0],
				loadctl.backoff_limit, loadctl.backoff_change,
				loadctl.rampup_limit, loadctl.rampup_change,
				loadctl.nproc_limit, loadctl.nofile_limit,
				loadctl.options, loadctl.changes
			);

			return 0;
		}

		else if (!strcmp(buf, "squeuestats")) {

			return dump_event_stats(sd);
		}
	}

	/* space != NULL: */
	else {

		len -= (unsigned long)(space - buf);

		if (!strcmp(buf, "loadctl")) {
			return set_loadctl_options(space, len) == OK ? 200 : 400;
		}
	}

	/* No matching command found */
	return 404;
}