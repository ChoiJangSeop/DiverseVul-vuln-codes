static char *getusername(void)
{
	char *username = NULL;
	struct passwd *password = getpwuid(getuid());

	if (password)
		username = password->pw_name;
	return username;
}