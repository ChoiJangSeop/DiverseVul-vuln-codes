parseuid(const char *s, uid_t *uid)
{
	struct passwd *pw;
	const char *errstr;

	if ((pw = getpwnam(s)) != NULL) {
		*uid = pw->pw_uid;
		return 0;
	}
	#if !defined(__linux__) && !defined(__NetBSD__)
	*uid = strtonum(s, 0, UID_MAX, &errstr);
	#else
	sscanf(s, "%d", uid);
	#endif
	if (errstr)
		return -1;
	return 0;
}