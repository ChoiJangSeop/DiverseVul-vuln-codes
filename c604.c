rfbNewTCPOrUDPClient(rfbScreenInfoPtr rfbScreen,
                     int sock,
                     rfbBool isUDP)
{
    rfbProtocolVersionMsg pv;
    rfbClientIteratorPtr iterator;
    rfbClientPtr cl,cl_;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    rfbProtocolExtension* extension;

    cl = (rfbClientPtr)calloc(sizeof(rfbClientRec),1);

    cl->screen = rfbScreen;
    cl->sock = sock;
    cl->viewOnly = FALSE;
    /* setup pseudo scaling */
    cl->scaledScreen = rfbScreen;
    cl->scaledScreen->scaledScreenRefCount++;

    rfbResetStats(cl);

    cl->clientData = NULL;
    cl->clientGoneHook = rfbDoNothingWithClient;

    if(isUDP) {
      rfbLog(" accepted UDP client\n");
    } else {
      int one=1;

      getpeername(sock, (struct sockaddr *)&addr, &addrlen);
      cl->host = strdup(inet_ntoa(addr.sin_addr));

      rfbLog("  other clients:\n");
      iterator = rfbGetClientIterator(rfbScreen);
      while ((cl_ = rfbClientIteratorNext(iterator)) != NULL) {
        rfbLog("     %s\n",cl_->host);
      }
      rfbReleaseClientIterator(iterator);

#ifndef WIN32
      if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	rfbLogPerror("fcntl failed");
	close(sock);
	return NULL;
      }
#endif

      if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		     (char *)&one, sizeof(one)) < 0) {
	rfbLogPerror("setsockopt failed");
	close(sock);
	return NULL;
      }

      FD_SET(sock,&(rfbScreen->allFds));
		rfbScreen->maxFd = max(sock,rfbScreen->maxFd);

      INIT_MUTEX(cl->outputMutex);
      INIT_MUTEX(cl->refCountMutex);
      INIT_COND(cl->deleteCond);

      cl->state = RFB_PROTOCOL_VERSION;

      cl->reverseConnection = FALSE;
      cl->readyForSetColourMapEntries = FALSE;
      cl->useCopyRect = FALSE;
      cl->preferredEncoding = -1;
      cl->correMaxWidth = 48;
      cl->correMaxHeight = 48;
#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->zrleData = NULL;
#endif

      cl->copyRegion = sraRgnCreate();
      cl->copyDX = 0;
      cl->copyDY = 0;
   
      cl->modifiedRegion =
	sraRgnCreateRect(0,0,rfbScreen->width,rfbScreen->height);

      INIT_MUTEX(cl->updateMutex);
      INIT_COND(cl->updateCond);

      cl->requestedRegion = sraRgnCreate();

      cl->format = cl->screen->serverFormat;
      cl->translateFn = rfbTranslateNone;
      cl->translateLookupTable = NULL;

      LOCK(rfbClientListMutex);

      IF_PTHREADS(cl->refCount = 0);
      cl->next = rfbScreen->clientHead;
      cl->prev = NULL;
      if (rfbScreen->clientHead)
        rfbScreen->clientHead->prev = cl;

      rfbScreen->clientHead = cl;
      UNLOCK(rfbClientListMutex);

#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->tightQualityLevel = -1;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
      cl->tightCompressLevel = TIGHT_DEFAULT_COMPRESSION;
      {
	int i;
	for (i = 0; i < 4; i++)
          cl->zsActive[i] = FALSE;
      }
#endif
#endif

      cl->fileTransfer.fd = -1;

      cl->enableCursorShapeUpdates = FALSE;
      cl->enableCursorPosUpdates = FALSE;
      cl->useRichCursorEncoding = FALSE;
      cl->enableLastRectEncoding = FALSE;
      cl->enableKeyboardLedState = FALSE;
      cl->enableSupportedMessages = FALSE;
      cl->enableSupportedEncodings = FALSE;
      cl->enableServerIdentity = FALSE;
      cl->lastKeyboardLedState = -1;
      cl->cursorX = rfbScreen->cursorX;
      cl->cursorY = rfbScreen->cursorY;
      cl->useNewFBSize = FALSE;

#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->compStreamInited = FALSE;
      cl->compStream.total_in = 0;
      cl->compStream.total_out = 0;
      cl->compStream.zalloc = Z_NULL;
      cl->compStream.zfree = Z_NULL;
      cl->compStream.opaque = Z_NULL;

      cl->zlibCompressLevel = 5;
#endif

      cl->progressiveSliceY = 0;

      cl->extensions = NULL;

      cl->lastPtrX = -1;

      sprintf(pv,rfbProtocolVersionFormat,rfbScreen->protocolMajorVersion, 
              rfbScreen->protocolMinorVersion);

      if (rfbWriteExact(cl, pv, sz_rfbProtocolVersionMsg) < 0) {
        rfbLogPerror("rfbNewClient: write");
        rfbCloseClient(cl);
	rfbClientConnectionGone(cl);
        return NULL;
      }
    }

    for(extension = rfbGetExtensionIterator(); extension;
	    extension=extension->next) {
	void* data = NULL;
	/* if the extension does not have a newClient method, it wants
	 * to be initialized later. */
	if(extension->newClient && extension->newClient(cl, &data))
		rfbEnableExtension(cl, extension, data);
    }
    rfbReleaseExtensionIterator();

    switch (cl->screen->newClientHook(cl)) {
    case RFB_CLIENT_ON_HOLD:
	    cl->onHold = TRUE;
	    break;
    case RFB_CLIENT_ACCEPT:
	    cl->onHold = FALSE;
	    break;
    case RFB_CLIENT_REFUSE:
	    rfbCloseClient(cl);
	    rfbClientConnectionGone(cl);
	    cl = NULL;
	    break;
    }
    return cl;
}