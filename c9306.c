_XimProtoSetICValues(
    XIC			 xic,
    XIMArg		*arg)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    XimDefICValues	 ic_values;
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
    XPointer		 preply = NULL;
    int			 ret_code;
    BITMASK32		 flag = 0L;
    char		*name;
    char		*tmp_name = (arg) ? arg->name : NULL;

#ifndef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic))
	return tmp_name;
#else
    if (!_XimSaveICValues(ic, arg))
	return NULL;

    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
	        if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
	            return _XimDelayModeSetICValues(ic, arg);
		}
	    } else {
	        return _XimDelayModeSetICValues(ic, arg);
	    }
        } else {
	    return tmp_name;
        }
    }
#endif /* XIM_CONNECTABLE */

    _XimGetCurrentICValues(ic, &ic_values);
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE
	+ sizeof(CARD16) + sizeof(CARD16) + sizeof(INT16) + sizeof(CARD16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    arg_ret = arg;
    for (;;) {
	data = &buf[buf_size];
	if ((name = _XimEncodeICATTRIBUTE(ic, ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, arg, &arg_ret,
			data, data_len, &ret_len, (XPointer)&ic_values,
			&flag, XIM_SETICVALUES))) {
	    break;
	}

	total += ret_len;
	if (!(arg = arg_ret)) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xmalloc(buf_size + data_len))) {
		return tmp_name;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return tmp_name;
	    }
	    buf = tmp;
	}
    }
    _XimSetCurrentICValues(ic, &ic_values);

    if (!total) {
        return tmp_name;
    }

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

#ifdef EXT_MOVE
    if (_XimExtenMove(im, ic, flag, &buf_s[4], (INT16)total))
	return name;
#endif

    buf_s[0] = im->private.proto.imid;
    buf_s[1] = ic->private.proto.icid;
    buf_s[2] = (INT16)total;
    buf_s[3] = 0;
    len = (INT16)(sizeof(CARD16) + sizeof(CARD16)
				+ sizeof(INT16) + sizeof(CARD16) + total);

    _XimSetHeader((XPointer)buf, XIM_SET_IC_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return tmp_name;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    ic->private.proto.waitCallback = True;
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimSetICValuesCheck, (XPointer)ic);
    if (ret_code == XIM_TRUE) {
	preply = reply;
    } else if (ret_code == XIM_OVERFLOW) {
	buf_size = (int)len;
	preply = Xmalloc(buf_size);
	ret_code = _XimRead(im, &len, preply, buf_size,
					_XimSetICValuesCheck, (XPointer)ic);
	if (ret_code != XIM_TRUE) {
	    Xfree(preply);
	    ic->private.proto.waitCallback = False;
	    return tmp_name;
	}
    } else {
	ic->private.proto.waitCallback = False;
	return tmp_name;
    }
    ic->private.proto.waitCallback = False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if (reply != preply)
	    Xfree(preply);
	return tmp_name;
    }
    if (reply != preply)
	Xfree(preply);

    return name;
}