XSetClassHint(
	Display *dpy,
	Window w,
	XClassHint *classhint)
{
	char *class_string;
	char *s;
	size_t len_nm, len_cl;

	len_nm = safestrlen(classhint->res_name);
	len_cl = safestrlen(classhint->res_class);
	if ((class_string = s = Xmalloc(len_nm + len_cl + 2))) {
	    if (len_nm) {
		strcpy(s, classhint->res_name);
		s += len_nm + 1;
	    }
	    else
		*s++ = '\0';
	    if (len_cl)
		strcpy(s, classhint->res_class);
	    else
		*s = '\0';
	    XChangeProperty(dpy, w, XA_WM_CLASS, XA_STRING, 8,
			    PropModeReplace, (unsigned char *) class_string,
			    len_nm+len_cl+2);
	    Xfree(class_string);
	}
	return 1;
}