static bool fbo_check_config(const char *cfgstring, char **reason)
{
	char *options;
	char *path;
	int fd;

	tcmu_dbg("check: cfgstring %s\n", cfgstring);
	options = strchr(cfgstring, '/');
	if (!options) {
		if (asprintf(reason, "Invalid cfgstring") == -1)
			*reason = NULL;
		return false;
	}
	options += 1; /* get past '/' */
	while (options[0] != '/') {
		if (strncasecmp(options, "ro/", 3)) {
			if (asprintf(reason, "Unknown option %s\n",
				     options) == -1)
				*reason = NULL;
			return false;
		}

		options = strchr(options, '/');
		if (!options) {
			if (asprintf(reason, "Invalid cfgstring") == -1)
				*reason = NULL;
			return false;
		}
		options += 1;
	}

	path = options;
	if (!path) {
		if (asprintf(reason, "No path found") == -1)
			*reason = NULL;
		return false;
	}

	if (access(path, R_OK) != -1)
		return true; /* File exists */

	/* We also support creating the file, so see if we can create it */
	/* NB: If we're creating it, then we'll need write permission */
	fd = creat(path, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		if (asprintf(reason, "Could not create file") == -1)
			*reason = NULL;
		return false;
	}

	unlink(path);

	return true;
}