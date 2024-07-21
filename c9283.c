rfbClient* rfbGetClient(int bitsPerSample,int samplesPerPixel,
			int bytesPerPixel) {
#ifdef WIN32
    WSADATA unused;
#endif
  rfbClient* client=(rfbClient*)calloc(sizeof(rfbClient),1);
  if(!client) {
    rfbClientErr("Couldn't allocate client structure!\n");
    return NULL;
  }
#ifdef WIN32
  if((errno = WSAStartup(MAKEWORD(2,0), &unused)) != 0) {
      rfbClientErr("Could not init Windows Sockets: %s\n", strerror(errno));
      return NULL;
  }
#endif
  initAppData(&client->appData);
  client->endianTest = 1;
  client->programName="";
  client->serverHost=strdup("");
  client->serverPort=5900;

  client->destHost = NULL;
  client->destPort = 5900;
  
  client->connectTimeout = DEFAULT_CONNECT_TIMEOUT;

  client->CurrentKeyboardLedState = 0;
  client->HandleKeyboardLedState = (HandleKeyboardLedStateProc)DummyPoint;

  /* default: use complete frame buffer */ 
  client->updateRect.x = -1;
 
  client->frameBuffer = NULL;
  client->outputWindow = 0;
 
  client->format.bitsPerPixel = bytesPerPixel*8;
  client->format.depth = bitsPerSample*samplesPerPixel;
  client->appData.requestedDepth=client->format.depth;
  client->format.bigEndian = *(char *)&client->endianTest?FALSE:TRUE;
  client->format.trueColour = 1;

  if (client->format.bitsPerPixel == 8) {
    client->format.redMax = 7;
    client->format.greenMax = 7;
    client->format.blueMax = 3;
    client->format.redShift = 0;
    client->format.greenShift = 3;
    client->format.blueShift = 6;
  } else {
    client->format.redMax = (1 << bitsPerSample) - 1;
    client->format.greenMax = (1 << bitsPerSample) - 1;
    client->format.blueMax = (1 << bitsPerSample) - 1;
    if(!client->format.bigEndian) {
      client->format.redShift = 0;
      client->format.greenShift = bitsPerSample;
      client->format.blueShift = bitsPerSample * 2;
    } else {
      if(client->format.bitsPerPixel==8*3) {
	client->format.redShift = bitsPerSample*2;
	client->format.greenShift = bitsPerSample*1;
	client->format.blueShift = 0;
      } else {
	client->format.redShift = bitsPerSample*3;
	client->format.greenShift = bitsPerSample*2;
	client->format.blueShift = bitsPerSample;
      }
    }
  }

  client->bufoutptr=client->buf;
  client->buffered=0;

#ifdef LIBVNCSERVER_HAVE_LIBZ
  client->raw_buffer_size = -1;
  client->decompStreamInited = FALSE;

#ifdef LIBVNCSERVER_HAVE_LIBJPEG
  memset(client->zlibStreamActive,0,sizeof(rfbBool)*4);
#endif
#endif

  client->HandleCursorPos = DummyPoint;
  client->SoftCursorLockArea = DummyRect;
  client->SoftCursorUnlockScreen = Dummy;
  client->GotFrameBufferUpdate = DummyRect;
  client->GotCopyRect = CopyRectangleFromRectangle;
  client->GotFillRect = FillRectangle;
  client->GotBitmap = CopyRectangle;
  client->FinishedFrameBufferUpdate = NULL;
  client->GetPassword = ReadPassword;
  client->MallocFrameBuffer = MallocFrameBuffer;
  client->Bell = Dummy;
  client->CurrentKeyboardLedState = 0;
  client->HandleKeyboardLedState = (HandleKeyboardLedStateProc)DummyPoint;
  client->QoS_DSCP = 0;

  client->authScheme = 0;
  client->subAuthScheme = 0;
  client->GetCredential = NULL;
  client->tlsSession = NULL;
  client->LockWriteToTLS = NULL;
  client->UnlockWriteToTLS = NULL;
  client->sock = RFB_INVALID_SOCKET;
  client->listenSock = RFB_INVALID_SOCKET;
  client->listenAddress = NULL;
  client->listen6Sock = RFB_INVALID_SOCKET;
  client->listen6Address = NULL;
  client->clientAuthSchemes = NULL;

#ifdef LIBVNCSERVER_HAVE_SASL
  client->GetSASLMechanism = NULL;
  client->GetUser = NULL;
  client->saslSecret = NULL;
#endif /* LIBVNCSERVER_HAVE_SASL */

  return client;
}