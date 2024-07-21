SetFormatAndEncodings(rfbClient* client)
{
  rfbSetPixelFormatMsg spf;
  char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];

  rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
  uint32_t *encs = (uint32_t *)(&buf[sz_rfbSetEncodingsMsg]);
  int len = 0;
  rfbBool requestCompressLevel = FALSE;
  rfbBool requestQualityLevel = FALSE;
  rfbBool requestLastRectEncoding = FALSE;
  rfbClientProtocolExtension* e;

  if (!SupportsClient2Server(client, rfbSetPixelFormat)) return TRUE;

  spf.type = rfbSetPixelFormat;
  spf.pad1 = 0;
  spf.pad2 = 0;
  spf.format = client->format;
  spf.format.redMax = rfbClientSwap16IfLE(spf.format.redMax);
  spf.format.greenMax = rfbClientSwap16IfLE(spf.format.greenMax);
  spf.format.blueMax = rfbClientSwap16IfLE(spf.format.blueMax);

  if (!WriteToRFBServer(client, (char *)&spf, sz_rfbSetPixelFormatMsg))
    return FALSE;


  if (!SupportsClient2Server(client, rfbSetEncodings)) return TRUE;

  se->type = rfbSetEncodings;
  se->pad = 0;
  se->nEncodings = 0;

  if (client->appData.encodingsString) {
    const char *encStr = client->appData.encodingsString;
    int encStrLen;
    do {
      const char *nextEncStr = strchr(encStr, ' ');
      if (nextEncStr) {
	encStrLen = nextEncStr - encStr;
	nextEncStr++;
      } else {
	encStrLen = strlen(encStr);
      }

      if (strncasecmp(encStr,"raw",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRaw);
      } else if (strncasecmp(encStr,"copyrect",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCopyRect);
#ifdef LIBVNCSERVER_HAVE_LIBZ
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
      } else if (strncasecmp(encStr,"tight",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingTight);
	requestLastRectEncoding = TRUE;
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
	if (client->appData.enableJPEG)
	  requestQualityLevel = TRUE;
#endif
#endif
      } else if (strncasecmp(encStr,"hextile",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingHextile);
#ifdef LIBVNCSERVER_HAVE_LIBZ
      } else if (strncasecmp(encStr,"zlib",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlib);
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
      } else if (strncasecmp(encStr,"zlibhex",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlibHex);
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
      } else if (strncasecmp(encStr,"trle",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingTRLE);
      } else if (strncasecmp(encStr,"zrle",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZRLE);
      } else if (strncasecmp(encStr,"zywrle",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZYWRLE);
	requestQualityLevel = TRUE;
#endif
      } else if ((strncasecmp(encStr,"ultra",encStrLen) == 0) || (strncasecmp(encStr,"ultrazip",encStrLen) == 0)) {
        /* There are 2 encodings used in 'ultra' */
        encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltra);
        encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltraZip);
      } else if (strncasecmp(encStr,"corre",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCoRRE);
      } else if (strncasecmp(encStr,"rre",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRRE);
      } else {
	rfbClientLog("Unknown encoding '%.*s'\n",encStrLen,encStr);
      }

      encStr = nextEncStr;
    } while (encStr && se->nEncodings < MAX_ENCODINGS);

    if (se->nEncodings < MAX_ENCODINGS && requestCompressLevel) {
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.compressLevel +
					  rfbEncodingCompressLevel0);
    }

    if (se->nEncodings < MAX_ENCODINGS && requestQualityLevel) {
      if (client->appData.qualityLevel < 0 || client->appData.qualityLevel > 9)
        client->appData.qualityLevel = 5;
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.qualityLevel +
					  rfbEncodingQualityLevel0);
    }
  }
  else {
    if (SameMachine(client->sock)) {
      /* TODO:
      if (!tunnelSpecified) {
      */
      rfbClientLog("Same machine: preferring raw encoding\n");
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRaw);
      /*
      } else {
	rfbClientLog("Tunneling active: preferring tight encoding\n");
      }
      */
    }

    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCopyRect);
#ifdef LIBVNCSERVER_HAVE_LIBZ
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingTight);
    requestLastRectEncoding = TRUE;
#endif
#endif
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingHextile);
#ifdef LIBVNCSERVER_HAVE_LIBZ
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlib);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZRLE);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZYWRLE);
#endif
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltra);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltraZip);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCoRRE);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRRE);

    if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9) {
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.compressLevel +
					  rfbEncodingCompressLevel0);
    } else /* if (!tunnelSpecified) */ {
      /* If -tunnel option was provided, we assume that server machine is
	 not in the local network so we use default compression level for
	 tight encoding instead of fast compression. Thus we are
	 requesting level 1 compression only if tunneling is not used. */
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCompressLevel1);
    }

    if (client->appData.enableJPEG) {
      if (client->appData.qualityLevel < 0 || client->appData.qualityLevel > 9)
	client->appData.qualityLevel = 5;
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.qualityLevel +
					  rfbEncodingQualityLevel0);
    }
  }



  /* Remote Cursor Support (local to viewer) */
  if (client->appData.useRemoteCursor) {
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingXCursor);
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRichCursor);
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingPointerPos);
  }

  /* Keyboard State Encodings */
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingKeyboardLedState);

  /* New Frame Buffer Size */
  if (se->nEncodings < MAX_ENCODINGS && client->canHandleNewFBSize)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingNewFBSize);

  /* Last Rect */
  if (se->nEncodings < MAX_ENCODINGS && requestLastRectEncoding)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingLastRect);

  /* Server Capabilities */
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingSupportedMessages);
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingSupportedEncodings);
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingServerIdentity);

  /* xvp */
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingXvp);

  /* client extensions */
  for(e = rfbClientExtensions; e; e = e->next)
    if(e->encodings) {
      int* enc;
      for(enc = e->encodings; *enc; enc++)
        if(se->nEncodings < MAX_ENCODINGS)
          encs[se->nEncodings++] = rfbClientSwap32IfLE(*enc);
    }

  len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

  se->nEncodings = rfbClientSwap16IfLE(se->nEncodings);

  if (!WriteToRFBServer(client, buf, len)) return FALSE;

  return TRUE;
}