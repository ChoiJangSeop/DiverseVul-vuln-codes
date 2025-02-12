int perf_config(config_fn_t fn, void *data)
{
	int ret = 0, found = 0;
	char *repo_config = NULL;
	const char *home = NULL;

	/* Setting $PERF_CONFIG makes perf read _only_ the given config file. */
	if (config_exclusive_filename)
		return perf_config_from_file(fn, config_exclusive_filename, data);
	if (perf_config_system() && !access(perf_etc_perfconfig(), R_OK)) {
		ret += perf_config_from_file(fn, perf_etc_perfconfig(),
					    data);
		found += 1;
	}

	home = getenv("HOME");
	if (perf_config_global() && home) {
		char *user_config = strdup(mkpath("%s/.perfconfig", home));
		if (!access(user_config, R_OK)) {
			ret += perf_config_from_file(fn, user_config, data);
			found += 1;
		}
		free(user_config);
	}

	repo_config = perf_pathdup("config");
	if (!access(repo_config, R_OK)) {
		ret += perf_config_from_file(fn, repo_config, data);
		found += 1;
	}
	free(repo_config);
	if (found == 0)
		return -1;
	return ret;
}