XSetFontPath (
    register Display *dpy,
    char **directories,
    int ndirs)
{
	register int n = 0;
	register int i;
	register int nbytes;
	char *p;
	register xSetFontPathReq *req;
	int retCode;

        LockDisplay(dpy);
	GetReq (SetFontPath, req);
	req->nFonts = ndirs;
	for (i = 0; i < ndirs; i++) {
		n = (int) ((size_t) n + (safestrlen (directories[i]) + 1));
	}
	nbytes = (n + 3) & ~3;
	req->length += nbytes >> 2;
	if ((p = Xmalloc (nbytes))) {
		/*
	 	 * pack into counted strings.
	 	 */
		char	*tmp = p;

		for (i = 0; i < ndirs; i++) {
			register int length = (int) safestrlen (directories[i]);
			*p = length;
			memcpy (p + 1, directories[i], (size_t)length);
			p += length + 1;
		}
		Data (dpy, tmp, nbytes);
		Xfree (tmp);
		retCode = 1;
	}
	else
		retCode = 0;

        UnlockDisplay(dpy);
	SyncHandle();
	return (retCode);
}