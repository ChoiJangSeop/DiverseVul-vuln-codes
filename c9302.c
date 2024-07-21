_XimProtoGetICValues(
    XIC			 xic,
    XIMArg		*arg)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    register XIMArg	*p;
    register XIMArg	*pp;
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
    if (!IS_IC_CONNECTED(ic))
	return arg->name;
#else
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
	            return _XimDelayModeGetICValues(ic, arg);
		}
	    } else {
	        return _XimDelayModeGetICValues(ic, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    for (n = 0, p = arg; p && p->name; p++) {
	n++;
	if ((strcmp(p->name, XNPreeditAttributes) == 0)
	 || (strcmp(p->name, XNStatusAttributes) == 0)) {
	     n++;
	     for (pp = (XIMArg *)p->value; pp && pp->name; pp++)
	 	n++;
	}
    }

    if (!n)
	return (char *)NULL;

    buf_size =  sizeof(CARD16) * n;
    buf_size += XIM_HEADER_SIZE
	     + sizeof(CARD16)
	     + sizeof(CARD16)
	     + sizeof(INT16)
	     + XIM_PAD(2 + buf_size);

    if (!(buf = Xmalloc(buf_size)))
	return arg->name;
    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    makeid_name = _XimMakeICAttrIDList(ic, ic->private.proto.ic_resources,
				ic->private.proto.ic_num_resources, arg,
				&buf_s[3], &len, XIM_GETICVALUES);

    if (len > 0) {
	buf_s[0] = im->private.proto.imid;		/* imid */
	buf_s[1] = ic->private.proto.icid;		/* icid */
	buf_s[2] = len;				/* length of ic-attr-id */
	len += sizeof(INT16);                       /* sizeof length of attr */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += sizeof(CARD16)			/* sizeof imid */
	     + sizeof(CARD16);			/* sizeof icid */

	_XimSetHeader((XPointer)buf, XIM_GET_IC_VALUES, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf))) {
	    Xfree(buf);
	    return arg->name;
	}
	_XimFlush(im);
	Xfree(buf);
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
				 _XimGetICValuesCheck, (XPointer)ic);
	if (ret_code == XIM_TRUE) {
	    preply = reply;
	} else if (ret_code == XIM_OVERFLOW) {
	    if (len <= 0) {
		preply = reply;
	    } else {
		buf_size = (int)len;
		preply = Xmalloc(len);
		ret_code = _XimRead(im, &len, preply, buf_size,
				_XimGetICValuesCheck, (XPointer)ic);
		if (ret_code != XIM_TRUE) {
		    if (preply != reply)
		        Xfree(preply);
		    return arg->name;
		}
	    }
	} else {
	    return arg->name;
	}
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
	if (*((CARD8 *)preply) == XIM_ERROR) {
	    _XimProcError(im, 0, (XPointer)&buf_s[3]);
	    if (reply != preply)
		Xfree(preply);
	    return arg->name;
	}
	data = &buf_s[4];
	data_len = buf_s[2];
    }
    else if (len < 0) {
	return arg->name;
    }

    decode_name = _XimDecodeICATTRIBUTE(ic, ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, data, data_len,
			arg, XIM_GETICVALUES);
    if (reply != preply)
	Xfree(preply);

    if (decode_name)
	return decode_name;
    else
	return makeid_name;
}