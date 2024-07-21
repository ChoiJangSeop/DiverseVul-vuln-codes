XSetCommand (
	Display *dpy,
	Window w,
	char **argv,
	int argc)
{
	register int i;
	size_t nbytes;
	register char *buf, *bp;
	for (i = 0, nbytes = 0; i < argc; i++) {
		nbytes += safestrlen(argv[i]) + 1;
	}
	if ((bp = buf = Xmalloc(nbytes))) {
	    /* copy arguments into single buffer */
	    for (i = 0; i < argc; i++) {
		if (argv[i]) {
		    (void) strcpy(bp, argv[i]);
		    bp += strlen(argv[i]) + 1;
		}
		else
		    *bp++ = '\0';
	    }
	    XChangeProperty (dpy, w, XA_WM_COMMAND, XA_STRING, 8,
			     PropModeReplace, (unsigned char *)buf, nbytes);
	    Xfree(buf);
	}
	return 1;
}