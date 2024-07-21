XFontStruct *XLoadQueryFont(
   register Display *dpy,
   _Xconst char *name)
{
    XFontStruct *font_result;
    register long nbytes;
    Font fid;
    xOpenFontReq *req;
    unsigned long seq;
#ifdef USE_XF86BIGFONT
    XF86BigfontCodes *extcodes = _XF86BigfontCodes(dpy);
#endif

    if (_XF86LoadQueryLocaleFont(dpy, name, &font_result, (Font *)0))
      return font_result;
    LockDisplay(dpy);
    GetReq(OpenFont, req);
    seq = dpy->request; /* Can't use extended sequence number here */
    nbytes = req->nbytes = (CARD16) (name ? strlen(name) : 0);
    req->fid = fid = XAllocID(dpy);
    req->length += (nbytes+3)>>2;
    Data (dpy, name, nbytes);
    font_result = NULL;
#ifdef USE_XF86BIGFONT
    if (extcodes) {
	font_result = _XF86BigfontQueryFont(dpy, extcodes, fid, seq);
	seq = 0;
    }
#endif
    if (!font_result)
	font_result = _XQueryFont(dpy, fid, seq);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}