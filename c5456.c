static bool qcow_check_config(const char *cfgstring, char **reason)
{
	char *path;

	path = strchr(cfgstring, '/');
	if (!path) {
		if (asprintf(reason, "No path found") == -1)
			*reason = NULL;
		return false;
	}
	path += 1; /* get past '/' */

	if (access(path, R_OK|W_OK) == -1) {
		if (asprintf(reason, "File not present, or not writable") == -1)
			*reason = NULL;
		return false;
	}

	return true; /* File exists and is writable */
}