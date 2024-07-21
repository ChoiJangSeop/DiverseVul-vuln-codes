parsegid(const char *s, gid_t *gid)
{
	struct group *gr;
	const char *errstr;

	if ((gr = getgrnam(s)) != NULL) {
		*gid = gr->gr_gid;
		return 0;
	}
	#if !defined(__linux__) && !defined(__NetBSD__)
	*gid = strtonum(s, 0, GID_MAX, &errstr);
	#else
	sscanf(s, "%d", gid);
	#endif
	if (errstr)
		return -1;
	return 0;
}