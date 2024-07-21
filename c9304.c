_XimEncodingNegotiation(
    Xim		 im)
{
    char	*name_ptr = 0;
    int		 name_len = 0;
    char	*detail_ptr = 0;
    int		 detail_len = 0;
    CARD8	*buf;
    CARD16	*buf_s;
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;

    if (!(_XimSetEncodingByName(im, &name_ptr, &name_len)))
	return False;

    if (!(_XimSetEncodingByDetail(im, &detail_ptr, &detail_len)))
	goto free_name_ptr;

    len = sizeof(CARD16)
	+ sizeof(INT16)
	+ name_len
	+ XIM_PAD(name_len)
	+ sizeof(INT16)
	+ sizeof(CARD16)
	+ detail_len;

    if (!(buf = Xmalloc(XIM_HEADER_SIZE + len)))
	goto free_detail_ptr;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)name_len;
    if (name_ptr)
	(void)memcpy((char *)&buf_s[2], name_ptr, name_len);
    XIM_SET_PAD(&buf_s[2], name_len);
    buf_s = (CARD16 *)((char *)&buf_s[2] + name_len);
    buf_s[0] = detail_len;
    buf_s[1] = 0;
    if (detail_ptr)
	(void)memcpy((char *)&buf_s[2], detail_ptr, detail_len);

    _XimSetHeader((XPointer)buf, XIM_ENCODING_NEGOTIATION, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	Xfree(buf);
	goto free_detail_ptr;
    }
    _XimFlush(im);
    Xfree(buf);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimEncodingNegoCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
					_XimEncodingNegoCheck, 0);
	    if(ret_code != XIM_TRUE)
		goto free_preply;
	}
    } else
	goto free_detail_ptr;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	goto free_preply;
    }

    if (!(_XimGetEncoding(im, &buf_s[1], name_ptr, name_len,
						detail_ptr, detail_len)))
	goto free_preply;

    Xfree(name_ptr);
    Xfree(detail_ptr);

    if(reply != preply)
	Xfree(preply);

    return True;

free_preply:
    if (reply != preply)
	Xfree(preply);

free_detail_ptr:
    Xfree(detail_ptr);

free_name_ptr:
    Xfree(name_ptr);

    return False;
}