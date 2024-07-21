XStoreNamedColor(
register Display *dpy,
Colormap cmap,
_Xconst char *name, /* STRING8 */
unsigned long pixel, /* CARD32 */
int flags)  /* DoRed, DoGreen, DoBlue */
{
    unsigned int nbytes;
    register xStoreNamedColorReq *req;
    XcmsCCC ccc;
    XcmsColor cmsColor_exact;
    XColor scr_def;

#ifdef XCMS
    /*
     * Let's Attempt to use Xcms approach to Parse Color
     */
    if ((ccc = XcmsCCCOfColormap(dpy, cmap)) != (XcmsCCC)NULL) {
	if (_XcmsResolveColorString(ccc, &name, &cmsColor_exact,
		XcmsRGBFormat) >= XcmsSuccess) {
	    _XcmsRGB_to_XColor(&cmsColor_exact, &scr_def, 1);
	    scr_def.pixel = pixel;
	    scr_def.flags = flags;
	    return XStoreColor(dpy, cmap, &scr_def);
	}
	/*
	 * Otherwise we failed; or name was changed with yet another
	 * name.  Thus pass name to the X Server.
	 */
    }
#endif

    /*
     * The Xcms and i18n methods failed, so lets pass it to the server
     * for parsing.
     */

    LockDisplay(dpy);
    GetReq(StoreNamedColor, req);

    req->cmap = cmap;
    req->flags = flags;
    req->pixel = pixel;
    req->nbytes = (CARD16) (nbytes = (unsigned) strlen(name));
    req->length += (nbytes + 3) >> 2; /* round up to multiple of 4 */
    Data(dpy, name, (long)nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
    return 0;
}