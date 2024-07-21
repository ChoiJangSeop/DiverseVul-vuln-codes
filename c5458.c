static bool file_check_config(const char *cfgstring, char **reason)
{
	char *path;
	int fd;

	path = strchr(cfgstring, '/');
	if (!path) {
		if (asprintf(reason, "No path found") == -1)
			*reason = NULL;
		return false;
	}
	path += 1; /* get past '/' */

	if (access(path, W_OK) != -1)
		return true; /* File exists and is writable */

	/* We also support creating the file, so see if we can create it */
	fd = creat(path, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		if (asprintf(reason, "Could not create file") == -1)
			*reason = NULL;
		return false;
	}

	unlink(path);

	return true;
}