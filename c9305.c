_XimProtoSetIMValues(
    XIM			 xim,
    XIMArg		*arg)
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    XIMArg		*arg_ret;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;
    char		*name;

#ifndef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im))
	return arg->name;
#else
    if (!_XimSaveIMValues(im, arg))
	return arg->name;

    if (!IS_SERVER_CONNECTED(im)) {
	if (IS_CONNECTABLE(im)) {
	    if (!_XimConnectServer(im)) {
	        return _XimDelayModeSetIMValues(im, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    _XimGetCurrentIMValues(im, &im_values);
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    arg_ret = arg;
    for (;;) {
	data = &buf[buf_size];
	if ((name = _XimEncodeIMATTRIBUTE(im, im->core.im_resources,
		im->core.im_num_resources, arg, &arg_ret, data, data_len,
		&ret_len, (XPointer)&im_values, XIM_SETIMVALUES))) {
	    break;
	}

	total += ret_len;
	if (!(arg = arg_ret)) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xmalloc(buf_size + data_len))) {
		return arg->name;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return arg->name;
	    }
	    buf = tmp;
	}
    }
    _XimSetCurrentIMValues(im, &im_values);

    if (!total)
	return (char *)NULL;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_SET_IM_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return arg->name;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					 _XimSetIMValuesCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = (int)len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, reply, buf_size,
					_XimSetIMValuesCheck, 0);
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
    if(reply != preply)
	Xfree(preply);

    return name;
}