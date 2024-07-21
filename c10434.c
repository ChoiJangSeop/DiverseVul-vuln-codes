XSetStandardProperties (
    	Display *dpy,
    	Window w,		/* window to decorate */
    	_Xconst char *name,	/* name of application */
    	_Xconst char *icon_string,/* name string for icon */
	Pixmap icon_pixmap,	/* pixmap to use as icon, or None */
    	char **argv,		/* command to be used to restart application */
    	int argc,		/* count of arguments */
    	XSizeHints *hints)	/* size hints for window in its normal state */
{
	XWMHints phints;
	phints.flags = 0;

	if (name != NULL) XStoreName (dpy, w, name);

	if (icon_string != NULL) {
	    XChangeProperty (dpy, w, XA_WM_ICON_NAME, XA_STRING, 8,
                             PropModeReplace,
                             (_Xconst unsigned char *)icon_string,
                             (int)safestrlen(icon_string));
		}

	if (icon_pixmap != None) {
		phints.icon_pixmap = icon_pixmap;
		phints.flags |= IconPixmapHint;
		}
	if (argv != NULL) XSetCommand(dpy, w, argv, argc);

	if (hints != NULL) XSetNormalHints(dpy, w, hints);

	if (phints.flags != 0) XSetWMHints(dpy, w, &phints);

	return 1;
}