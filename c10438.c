XStoreName (
    register Display *dpy,
    Window w,
    _Xconst char *name)
{
    return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING,
			   8, PropModeReplace, (_Xconst unsigned char *)name,
			   name ? (int) strlen(name) : 0);
}