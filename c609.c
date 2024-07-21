rfbBool rfbSendRectEncodingZRLE(rfbClientPtr cl, int x, int y, int w, int h)
{
  zrleOutStream* zos;
  rfbFramebufferUpdateRectHeader rect;
  rfbZRLEHeader hdr;
  int i;

  if (cl->preferredEncoding == rfbEncodingZYWRLE) {
	  if (cl->tightQualityLevel < 0) {
		  cl->zywrleLevel = 1;
	  } else if (cl->tightQualityLevel < 3) {
		  cl->zywrleLevel = 3;
	  } else if (cl->tightQualityLevel < 6) {
		  cl->zywrleLevel = 2;
	  } else {
		  cl->zywrleLevel = 1;
	  }
  } else
	  cl->zywrleLevel = 0;

  if (!cl->zrleData)
    cl->zrleData = zrleOutStreamNew();
  zos = cl->zrleData;
  zos->in.ptr = zos->in.start;
  zos->out.ptr = zos->out.start;

  switch (cl->format.bitsPerPixel) {

  case 8:
    zrleEncode8NE(x, y, w, h, zos, zrleBeforeBuf, cl);
    break;

  case 16:
	if (cl->format.greenMax > 0x1F) {
		if (cl->format.bigEndian)
		  zrleEncode16BE(x, y, w, h, zos, zrleBeforeBuf, cl);
		else
		  zrleEncode16LE(x, y, w, h, zos, zrleBeforeBuf, cl);
	} else {
		if (cl->format.bigEndian)
		  zrleEncode15BE(x, y, w, h, zos, zrleBeforeBuf, cl);
		else
		  zrleEncode15LE(x, y, w, h, zos, zrleBeforeBuf, cl);
	}
    break;

  case 32: {
    rfbBool fitsInLS3Bytes
      = ((cl->format.redMax   << cl->format.redShift)   < (1<<24) &&
         (cl->format.greenMax << cl->format.greenShift) < (1<<24) &&
         (cl->format.blueMax  << cl->format.blueShift)  < (1<<24));

    rfbBool fitsInMS3Bytes = (cl->format.redShift   > 7  &&
                           cl->format.greenShift > 7  &&
                           cl->format.blueShift  > 7);

    if ((fitsInLS3Bytes && !cl->format.bigEndian) ||
        (fitsInMS3Bytes && cl->format.bigEndian)) {
	if (cl->format.bigEndian)
		zrleEncode24ABE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode24ALE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
    else if ((fitsInLS3Bytes && cl->format.bigEndian) ||
             (fitsInMS3Bytes && !cl->format.bigEndian)) {
	if (cl->format.bigEndian)
		zrleEncode24BBE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode24BLE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
    else {
	if (cl->format.bigEndian)
		zrleEncode32BE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode32LE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
  }
    break;
  }

  rfbStatRecordEncodingSent(cl, rfbEncodingZRLE, sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader + ZRLE_BUFFER_LENGTH(&zos->out),
      + w * (cl->format.bitsPerPixel / 8) * h);

  if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader
      > UPDATE_BUF_SIZE)
    {
      if (!rfbSendUpdateBuf(cl))
        return FALSE;
    }

  rect.r.x = Swap16IfLE(x);
  rect.r.y = Swap16IfLE(y);
  rect.r.w = Swap16IfLE(w);
  rect.r.h = Swap16IfLE(h);
  rect.encoding = Swap32IfLE(cl->preferredEncoding);

  memcpy(cl->updateBuf+cl->ublen, (char *)&rect,
         sz_rfbFramebufferUpdateRectHeader);
  cl->ublen += sz_rfbFramebufferUpdateRectHeader;

  hdr.length = Swap32IfLE(ZRLE_BUFFER_LENGTH(&zos->out));

  memcpy(cl->updateBuf+cl->ublen, (char *)&hdr, sz_rfbZRLEHeader);
  cl->ublen += sz_rfbZRLEHeader;

  /* copy into updateBuf and send from there.  Maybe should send directly? */

  for (i = 0; i < ZRLE_BUFFER_LENGTH(&zos->out);) {

    int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

    if (i + bytesToCopy > ZRLE_BUFFER_LENGTH(&zos->out)) {
      bytesToCopy = ZRLE_BUFFER_LENGTH(&zos->out) - i;
    }

    memcpy(cl->updateBuf+cl->ublen, (uint8_t*)zos->out.start + i, bytesToCopy);

    cl->ublen += bytesToCopy;
    i += bytesToCopy;

    if (cl->ublen == UPDATE_BUF_SIZE) {
      if (!rfbSendUpdateBuf(cl))
        return FALSE;
    }
  }

  return TRUE;
}