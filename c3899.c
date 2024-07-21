int _gnutls_find_config_path(char *path, size_t max_size)
{
	const char *home_dir = getenv("HOME");

	if (home_dir != NULL && home_dir[0] != 0) {
		snprintf(path, max_size, "%s/" CONFIG_PATH, home_dir);
		return 0;
	}

#ifdef _WIN32
	if (home_dir == NULL || home_dir[0] == '\0') {
		const char *home_drive = getenv("HOMEDRIVE");
		const char *home_path = getenv("HOMEPATH");

		if (home_drive != NULL && home_path != NULL) {
			snprintf(path, max_size, "%s%s/" CONFIG_PATH, home_drive, home_path);
		} else {
			path[0] = 0;
		}
	}
#elif defined(HAVE_GETPWUID_R)
	if (home_dir == NULL || home_dir[0] == '\0') {
		struct passwd *pwd;
		struct passwd _pwd;
		int ret;
		char tmp[512];

		ret = getpwuid_r(getuid(), &_pwd, tmp, sizeof(tmp), &pwd);
		if (ret == 0 && pwd != NULL) {
			snprintf(path, max_size, "%s/" CONFIG_PATH, pwd->pw_dir);
		} else {
			path[0] = 0;
		}
	}
#else
	if (home_dir == NULL || home_dir[0] == '\0') {
			path[0] = 0;
	}
#endif

	return 0;
}