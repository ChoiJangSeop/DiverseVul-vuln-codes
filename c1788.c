pg_open_tzfile(const char *name, char *canonname)
{
	char		fullname[MAXPGPATH];

	if (canonname)
		strlcpy(canonname, name, TZ_STRLEN_MAX + 1);

	strcpy(fullname, pg_TZDIR());
	if (strlen(fullname) + 1 + strlen(name) >= MAXPGPATH)
		return -1;				/* not gonna fit */
	strcat(fullname, "/");
	strcat(fullname, name);

	return open(fullname, O_RDONLY | PG_BINARY, 0);
}