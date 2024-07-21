int _XF86LoadQueryLocaleFont(
   Display *dpy,
   _Xconst char *name,
   XFontStruct **xfp,
   Font *fidp)
{
    int l;
    const char *charset, *p;
    char buf[256];
    XFontStruct *fs;
    XLCd lcd;

    if (!name)
	return 0;
    l = (int) strlen(name);
    if (l < 2 || name[l - 1] != '*' || name[l - 2] != '-')
	return 0;
    charset = NULL;
    /* next three lines stolen from _XkbGetCharset() */
    lcd = _XlcCurrentLC();
    if ((lcd = _XlcCurrentLC()) != 0)
	charset = XLC_PUBLIC(lcd, encoding_name);
    if (!charset || (p = strrchr(charset, '-')) == 0 || p == charset || p[1] == 0 || (p[1] == '*' && p[2] == 0)) {
	/* prefer latin1 if no encoding found */
	charset = "ISO8859-1";
	p = charset + 7;
    }
    if (l - 2 - (p - charset) < 0)
	return 0;
    if (_XlcNCompareISOLatin1(name + l - 2 - (p - charset), charset, p - charset))
	return 0;
    if (strlen(p + 1) + (size_t) l - 1 >= sizeof(buf) - 1)
	return 0;
    strcpy(buf, name);
    strcpy(buf + l - 1, p + 1);
    fs = XLoadQueryFont(dpy, buf);
    if (!fs)
	return 0;
    if (xfp) {
	*xfp = fs;
	if (fidp)
	    *fidp = fs->fid;
    } else if (fidp) {
	if (fs->per_char) {
#ifdef USE_XF86BIGFONT
	    _XF86BigfontFreeFontMetrics(fs);
#else
	    Xfree (fs->per_char);
#endif
	}
	_XFreeExtData(fs->ext_data);

	Xfree (fs->properties);
	*fidp = fs->fid;
	Xfree (fs);
    } else {
	XFreeFont(dpy, fs);
    }
    return 1;
}