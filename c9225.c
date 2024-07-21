webSocketsDecodeHybi(rfbClientPtr cl, char *dst, int len)
{
    char *buf, *payload;
    uint32_t *payload32;
    int ret = -1, result = -1;
    int total = 0;
    ws_mask_t mask;
    ws_header_t *header;
    int i;
    unsigned char opcode;
    ws_ctx_t *wsctx = (ws_ctx_t *)cl->wsctx;
    int flength, fhlen;
    /* int fin; */ /* not used atm */ 

    /* rfbLog(" <== %s[%d]: %d cl: %p, wsctx: %p-%p (%d)\n", __func__, gettid(), len, cl, wsctx, (char *)wsctx + sizeof(ws_ctx_t), sizeof(ws_ctx_t)); */

    if (wsctx->readbuflen) {
      /* simply return what we have */
      if (wsctx->readbuflen > len) {
	memcpy(dst, wsctx->readbuf +  wsctx->readbufstart, len);
	result = len;
	wsctx->readbuflen -= len;
	wsctx->readbufstart += len;
      } else {
	memcpy(dst, wsctx->readbuf +  wsctx->readbufstart, wsctx->readbuflen);
	result = wsctx->readbuflen;
	wsctx->readbuflen = 0;
	wsctx->readbufstart = 0;
      }
      goto spor;
    }

    buf = wsctx->codeBufDecode;
    header = (ws_header_t *)wsctx->codeBufDecode;

    ret = ws_peek(cl, buf, B64LEN(len) + WSHLENMAX);

    if (ret < 2) {
        /* save errno because rfbErr() will tamper it */
        if (-1 == ret) {
            int olderrno = errno;
            rfbErr("%s: peek; %m\n", __func__);
            errno = olderrno;
        } else if (0 == ret) {
            result = 0;
        } else {
            errno = EAGAIN;
        }
        goto spor;
    }

    opcode = header->b0 & 0x0f;
    /* fin = (header->b0 & 0x80) >> 7; */ /* not used atm */
    flength = header->b1 & 0x7f;

    /*
     * 4.3. Client-to-Server Masking
     *
     * The client MUST mask all frames sent to the server.  A server MUST
     * close the connection upon receiving a frame with the MASK bit set to 0.
    **/
    if (!(header->b1 & 0x80)) {
	rfbErr("%s: got frame without mask\n", __func__, ret);
	errno = EIO;
	goto spor;
    }

    if (flength < 126) {
	fhlen = 2;
	mask = header->u.m;
    } else if (flength == 126 && 4 <= ret) {
	flength = WS_NTOH16(header->u.s16.l16);
	fhlen = 4;
	mask = header->u.s16.m16;
    } else if (flength == 127 && 10 <= ret) {
	flength = WS_NTOH64(header->u.s64.l64);
	fhlen = 10;
	mask = header->u.s64.m64;
    } else {
      /* Incomplete frame header */
      rfbErr("%s: incomplete frame header\n", __func__, ret);
      errno = EIO;
      goto spor;
    }

    /* absolute length of frame */
    total = fhlen + flength + 4;
    payload = buf + fhlen + 4; /* header length + mask */

    if (-1 == (ret = ws_read(cl, buf, total))) {
      int olderrno = errno;
      rfbErr("%s: read; %m", __func__);
      errno = olderrno;
      return ret;
    } else if (ret < total) {
      /* GT TODO: hmm? */
      rfbLog("%s: read; got partial data\n", __func__);
    } else {
      buf[ret] = '\0';
    }

    /* process 1 frame (32 bit op) */
    payload32 = (uint32_t *)payload;
    for (i = 0; i < flength / 4; i++) {
	payload32[i] ^= mask.u;
    }
    /* process the remaining bytes (if any) */
    for (i*=4; i < flength; i++) {
	payload[i] ^= mask.c[i % 4];
    }

    switch (opcode) {
      case WS_OPCODE_CLOSE:
	rfbLog("got closure, reason %d\n", WS_NTOH16(((uint16_t *)payload)[0]));
	errno = ECONNRESET;
	break;
      case WS_OPCODE_TEXT_FRAME:
	if (-1 == (flength = b64_pton(payload, (unsigned char *)wsctx->codeBufDecode, sizeof(wsctx->codeBufDecode)))) {
	  rfbErr("%s: Base64 decode error; %m\n", __func__);
	  break;
	}
	payload = wsctx->codeBufDecode;
	/* fall through */
      case WS_OPCODE_BINARY_FRAME:
	if (flength > len) {
	  memcpy(wsctx->readbuf, payload + len, flength - len);
	  wsctx->readbufstart = 0;
	  wsctx->readbuflen = flength - len;
	  flength = len;
	}
	memcpy(dst, payload, flength);
	result = flength;
	break;
      default:
	rfbErr("%s: unhandled opcode %d, b0: %02x, b1: %02x\n", __func__, (int)opcode, header->b0, header->b1);
    }

    /* single point of return, if someone has questions :-) */
spor:
    /* rfbLog("%s: ret: %d/%d\n", __func__, result, len); */
    return result;
}