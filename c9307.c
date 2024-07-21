_XimSendSavedIMValues(
    Xim			 im)
{
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
    int			 idx;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;

    _XimGetCurrentIMValues(im, &im_values);
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    idx = 0;
    for (;;) {
	data = &buf[buf_size];
	if (!_XimEncodeSavedIMATTRIBUTE(im, im->core.im_resources,
		im->core.im_num_resources, &idx, data, data_len,
		&ret_len, (XPointer)&im_values, XIM_SETIMVALUES)) {
	    if (buf != tmp_buf)
		Xfree(buf);
	    return False;
	}

	total += ret_len;
	if (idx == -1) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xmalloc(buf_size + data_len))) {
		return False;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return False;
	    }
	    buf = tmp;
	}
    }

    if (!total)
	return True;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_SET_IM_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return False;
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
    if(reply != preply)
	Xfree(preply);

    return True;
}