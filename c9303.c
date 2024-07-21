_XimProtoGetIMValues(
    XIM			 xim,
    XIMArg		*arg)
{
    Xim			 im = (Xim)xim;
    register XIMArg	*p;
    register int	 n;
    CARD8		*buf;
    CARD16		*buf_s;
    INT16		 len;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply = NULL;
    int			 buf_size;
    int			 ret_code;
    char		*makeid_name;
    char		*decode_name;
    CARD16		*data = NULL;
    INT16		 data_len = 0;

#ifndef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im))
	return arg->name;
#else
    if (!IS_SERVER_CONNECTED(im)) {
	if (IS_CONNECTABLE(im)) {
	    if (!_XimConnectServer(im)) {
	        return _XimDelayModeGetIMValues(im, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    for (n = 0, p = arg; p->name; p++)
	n++;

    if (!n)
	return (char *)NULL;

    buf_size =  sizeof(CARD16) * n;
    buf_size += XIM_HEADER_SIZE
	     + sizeof(CARD16)
	     + sizeof(INT16)
	     + XIM_PAD(buf_size);

    if (!(buf = Xmalloc(buf_size)))
	return arg->name;
    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    makeid_name = _XimMakeIMAttrIDList(im, im->core.im_resources,
				im->core.im_num_resources, arg,
				&buf_s[2], &len, XIM_GETIMVALUES);

    if (len) {
	buf_s[0] = im->private.proto.imid;	/* imid */
	buf_s[1] = len;				/* length of im-attr-id */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += sizeof(CARD16)			/* sizeof imid */
	     + sizeof(INT16);			/* sizeof length of attr */

	_XimSetHeader((XPointer)buf, XIM_GET_IM_VALUES, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf))) {
	    Xfree(buf);
	    return arg->name;
	}
	_XimFlush(im);
	Xfree(buf);
    	buf_size = BUFSIZE;
    	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    						_XimGetIMValuesCheck, 0);
	if(ret_code == XIM_TRUE) {
	    preply = reply;
	} else if(ret_code == XIM_OVERFLOW) {
	    if(len <= 0) {
		preply = reply;
	    } else {
		buf_size = len;
		preply = Xmalloc(buf_size);
		ret_code = _XimRead(im, &len, preply, buf_size,
						_XimGetIMValuesCheck, 0);
		if(ret_code != XIM_TRUE) {
		    Xfree(preply);
		    return arg->name;
		}
	    }
	} else
	    return arg->name;
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
	if (*((CARD8 *)preply) == XIM_ERROR) {
	    _XimProcError(im, 0, (XPointer)&buf_s[3]);
	    if(reply != preply)
		Xfree(preply);
	    return arg->name;
	}
	data = &buf_s[2];
	data_len = buf_s[1];
    }
    decode_name = _XimDecodeIMATTRIBUTE(im, im->core.im_resources,
			im->core.im_num_resources, data, data_len,
			arg, XIM_GETIMVALUES);
    if (reply != preply)
	Xfree(preply);

    if (decode_name)
	return decode_name;
    else
	return makeid_name;
}