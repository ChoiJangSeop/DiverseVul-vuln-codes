DLLIMPORT char *cfg_tilde_expand(const char *filename)
{
	char *expanded = NULL;

#ifndef _WIN32
	/* Do tilde expansion */
	if (filename[0] == '~') {
		struct passwd *passwd = NULL;
		const char *file = NULL;

		if (filename[1] == '/' || filename[1] == 0) {
			/* ~ or ~/path */
			passwd = getpwuid(geteuid());
			file = filename + 1;
		} else {
			/* ~user or ~user/path */
			char *user;

			file = strchr(filename, '/');
			if (file == NULL)
				file = filename + strlen(filename);

			user = malloc(file - filename);
			if (!user)
				return NULL;

			strncpy(user, filename + 1, file - filename - 1);
			passwd = getpwnam(user);
			free(user);
		}

		if (passwd) {
			expanded = malloc(strlen(passwd->pw_dir) + strlen(file) + 1);
			if (!expanded)
				return NULL;

			strcpy(expanded, passwd->pw_dir);
			strcat(expanded, file);
		}
	}
#endif
	if (!expanded)
		expanded = strdup(filename);

	return expanded;
}