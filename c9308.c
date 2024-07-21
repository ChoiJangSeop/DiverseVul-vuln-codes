_XimOpen(
    Xim			 im)
{
    CARD32		 buf32[BUFSIZE/4];
    CARD8		*buf = (CARD8 *)buf32;
    CARD8		*buf_b = &buf[XIM_HEADER_SIZE];
    CARD16		*buf_s;
    INT16		 len;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 buf_size;
    int			 ret_code;
    char		*locale_name;

    locale_name = im->private.proto.locale_name;
    len = strlen(locale_name);
    buf_b[0] = (BYTE)len;			   /* length of locale name */
    (void)strcpy((char *)&buf_b[1], locale_name);  /* locale name */
    len += sizeof(BYTE);			   /* sizeof length */
    XIM_SET_PAD(buf_b, len);			   /* pad */

    _XimSetHeader((XPointer)buf, XIM_OPEN, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, reply, buf_size,
					_XimOpenCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
					_XimOpenCheck, 0);
	    if(ret_code != XIM_TRUE) {
		Xfree(preply);
		return False;
	    }
	}
    } else
	return False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if(reply != preply)
	    Xfree(preply);
	return False;
    }

    im->private.proto.imid = buf_s[0];		/* imid */

    if (!(_XimGetAttributeID(im, &buf_s[1]))) {
	if(reply != preply)
	    Xfree(preply);
	return False;
    }
    if(reply != preply)
	Xfree(preply);

    if (!(_XimSetInnerIMResourceList(&(im->private.proto.im_inner_resources),
				&(im->private.proto.im_num_inner_resources))))
	return False;

    if (!(_XimSetInnerICResourceList(&(im->private.proto.ic_inner_resources),
				&(im->private.proto.ic_num_inner_resources))))
	return False;

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);
    _XimSetIMMode(im->private.proto.im_inner_resources,
				im->private.proto.im_num_inner_resources);

    /* Transport Callback */
    _XimRegProtoIntrCallback(im, XIM_SET_EVENT_MASK, 0,
				 _XimSetEventMaskCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_FORWARD_EVENT, 0,
				 _XimForwardEventCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_COMMIT, 0,
				 _XimCommitCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_SYNC, 0,
				 _XimSyncCallback, (XPointer)im);

    if(!_XimExtension(im))
	return False;

    /* register a hook for callback protocols */
    _XimRegisterDispatcher(im, _XimCbDispatch, (XPointer)im);

    return True;
}