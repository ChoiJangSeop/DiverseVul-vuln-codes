static void rfbProcessClientNormalMessage(rfbClientPtr cl)
{
  int n;
  rfbClientToServerMsg msg;
  char *str;

  READ((char *)&msg, 1)

  switch (msg.type) {

    case rfbSetPixelFormat:

      READ(((char *)&msg) + 1, sz_rfbSetPixelFormatMsg - 1)

      cl->format.bitsPerPixel = msg.spf.format.bitsPerPixel;
      cl->format.depth = msg.spf.format.depth;
      cl->format.bigEndian = (msg.spf.format.bigEndian ? 1 : 0);
      cl->format.trueColour = (msg.spf.format.trueColour ? 1 : 0);
      cl->format.redMax = Swap16IfLE(msg.spf.format.redMax);
      cl->format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
      cl->format.blueMax = Swap16IfLE(msg.spf.format.blueMax);
      cl->format.redShift = msg.spf.format.redShift;
      cl->format.greenShift = msg.spf.format.greenShift;
      cl->format.blueShift = msg.spf.format.blueShift;

      cl->readyForSetColourMapEntries = TRUE;

      rfbSetTranslateFunction(cl);
      return;

    case rfbFixColourMapEntries:
      READ(((char *)&msg) + 1, sz_rfbFixColourMapEntriesMsg - 1)
      rfbLog("rfbProcessClientNormalMessage: FixColourMapEntries unsupported\n");
      rfbCloseClient(cl);
      return;

    case rfbSetEncodings:
    {
      int i;
      CARD32 enc;
      Bool firstFence = !cl->enableFence;
      Bool firstCU = !cl->enableCU;
      Bool firstGII = !cl->enableGII;
      Bool logTightCompressLevel = FALSE;

      READ(((char *)&msg) + 1, sz_rfbSetEncodingsMsg - 1)

      msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);

      cl->preferredEncoding = -1;
      cl->useCopyRect = FALSE;
      cl->enableCursorShapeUpdates = FALSE;
      cl->enableCursorPosUpdates = FALSE;
      cl->enableLastRectEncoding = FALSE;
      cl->tightCompressLevel = TIGHT_DEFAULT_COMPRESSION;
      cl->tightSubsampLevel = TIGHT_DEFAULT_SUBSAMP;
      cl->tightQualityLevel = -1;
      cl->imageQualityLevel = -1;

      for (i = 0; i < msg.se.nEncodings; i++) {
        READ((char *)&enc, 4)
        enc = Swap32IfLE(enc);

        switch (enc) {

          case rfbEncodingCopyRect:
            cl->useCopyRect = TRUE;
            break;
          case rfbEncodingRaw:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using raw encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingRRE:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using rre encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingCoRRE:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using CoRRE encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingHextile:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using hextile encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingZlib:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using zlib encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingZRLE:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using ZRLE encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingZYWRLE:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using ZYWRLE encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingTight:
            if (cl->preferredEncoding == -1) {
              cl->preferredEncoding = enc;
              rfbLog("Using tight encoding for client %s\n", cl->host);
            }
            break;
          case rfbEncodingXCursor:
            if (!cl->enableCursorShapeUpdates) {
              rfbLog("Enabling X-style cursor updates for client %s\n",
                     cl->host);
              cl->enableCursorShapeUpdates = TRUE;
              cl->useRichCursorEncoding = FALSE;
              cl->cursorWasChanged = TRUE;
            }
            break;
          case rfbEncodingRichCursor:
            if (!cl->enableCursorShapeUpdates) {
              rfbLog("Enabling full-color cursor updates for client %s\n",
                     cl->host);
              cl->enableCursorShapeUpdates = TRUE;
              cl->useRichCursorEncoding = TRUE;
              cl->cursorWasChanged = TRUE;
            }
            break;
          case rfbEncodingPointerPos:
            if (!cl->enableCursorPosUpdates) {
              rfbLog("Enabling cursor position updates for client %s\n",
                     cl->host);
              cl->enableCursorPosUpdates = TRUE;
              cl->cursorWasMoved = TRUE;
              cl->cursorX = -1;
              cl->cursorY = -1;
            }
            break;
          case rfbEncodingLastRect:
            if (!cl->enableLastRectEncoding) {
              rfbLog("Enabling LastRect protocol extension for client %s\n",
                     cl->host);
              cl->enableLastRectEncoding = TRUE;
            }
            break;
          case rfbEncodingFence:
            if (!cl->enableFence) {
              rfbLog("Enabling Fence protocol extension for client %s\n",
                     cl->host);
              cl->enableFence = TRUE;
            }
            break;
          case rfbEncodingContinuousUpdates:
            if (!cl->enableCU) {
              rfbLog("Enabling Continuous Updates protocol extension for client %s\n",
                     cl->host);
              cl->enableCU = TRUE;
            }
            break;
          case rfbEncodingNewFBSize:
            if (!cl->enableDesktopSize) {
              if (!rfbAuthDisableRemoteResize) {
                rfbLog("Enabling Desktop Size protocol extension for client %s\n",
                       cl->host);
                cl->enableDesktopSize = TRUE;
              } else
                rfbLog("WARNING: Remote desktop resizing disabled per system policy.\n");
            }
            break;
          case rfbEncodingExtendedDesktopSize:
            if (!cl->enableExtDesktopSize) {
              if (!rfbAuthDisableRemoteResize) {
                rfbLog("Enabling Extended Desktop Size protocol extension for client %s\n",
                       cl->host);
                cl->enableExtDesktopSize = TRUE;
              } else
                rfbLog("WARNING: Remote desktop resizing disabled per system policy.\n");
            }
            break;
          case rfbEncodingGII:
            if (!cl->enableGII) {
              rfbLog("Enabling GII extension for client %s\n", cl->host);
              cl->enableGII = TRUE;
            }
            break;
          default:
            if (enc >= (CARD32)rfbEncodingCompressLevel0 &&
                enc <= (CARD32)rfbEncodingCompressLevel9) {
              cl->zlibCompressLevel = enc & 0x0F;
              cl->tightCompressLevel = enc & 0x0F;
              if (cl->preferredEncoding == rfbEncodingTight)
                logTightCompressLevel = TRUE;
              else
                rfbLog("Using compression level %d for client %s\n",
                       cl->tightCompressLevel, cl->host);
              if (rfbInterframe == -1) {
                if (cl->tightCompressLevel >= 5) {
                  if (!InterframeOn(cl)) {
                    rfbCloseClient(cl);
                    return;
                  }
                } else
                  InterframeOff(cl);
              }
            } else if (enc >= (CARD32)rfbEncodingSubsamp1X &&
                       enc <= (CARD32)rfbEncodingSubsampGray) {
              cl->tightSubsampLevel = enc & 0xFF;
              rfbLog("Using JPEG subsampling %d for client %s\n",
                     cl->tightSubsampLevel, cl->host);
            } else if (enc >= (CARD32)rfbEncodingQualityLevel0 &&
                       enc <= (CARD32)rfbEncodingQualityLevel9) {
              cl->tightQualityLevel = JPEG_QUAL[enc & 0x0F];
              cl->tightSubsampLevel = JPEG_SUBSAMP[enc & 0x0F];
              cl->imageQualityLevel = enc & 0x0F;
              if (cl->preferredEncoding == rfbEncodingTight)
                rfbLog("Using JPEG subsampling %d, Q%d for client %s\n",
                       cl->tightSubsampLevel, cl->tightQualityLevel, cl->host);
              else
                rfbLog("Using image quality level %d for client %s\n",
                       cl->imageQualityLevel, cl->host);
            } else if (enc >= (CARD32)rfbEncodingFineQualityLevel0 + 1 &&
                       enc <= (CARD32)rfbEncodingFineQualityLevel100) {
              cl->tightQualityLevel = enc & 0xFF;
              rfbLog("Using JPEG quality %d for client %s\n",
                     cl->tightQualityLevel, cl->host);
            } else {
              rfbLog("rfbProcessClientNormalMessage: ignoring unknown encoding %d (%x)\n",
                     (int)enc, (int)enc);
            }
        }  /* switch (enc) */
      }  /* for (i = 0; i < msg.se.nEncodings; i++) */

      if (cl->preferredEncoding == -1)
        cl->preferredEncoding = rfbEncodingTight;

      if (cl->preferredEncoding == rfbEncodingTight && logTightCompressLevel)
        rfbLog("Using Tight compression level %d for client %s\n",
               rfbTightCompressLevel(cl), cl->host);

      if (cl->enableCursorPosUpdates && !cl->enableCursorShapeUpdates) {
        rfbLog("Disabling cursor position updates for client %s\n", cl->host);
        cl->enableCursorPosUpdates = FALSE;
      }

      if (cl->enableFence && firstFence) {
        if (!rfbSendFence(cl, rfbFenceFlagRequest, 0, NULL))
          return;
      }

      if (cl->enableCU && cl->enableFence && firstCU) {
        if (!rfbSendEndOfCU(cl))
          return;
      }

      if (cl->enableGII && firstGII) {
        /* Send GII server version message to all clients */
        rfbGIIServerVersionMsg msg;

        msg.type = rfbGIIServer;
        /* We always send as big endian to make things easier on the Java
           viewer. */
        msg.endianAndSubType = rfbGIIVersion | rfbGIIBE;
        msg.length = Swap16IfLE(sz_rfbGIIServerVersionMsg - 4);
        msg.maximumVersion = msg.minimumVersion = Swap16IfLE(1);

        if (WriteExact(cl, (char *)&msg, sz_rfbGIIServerVersionMsg) < 0) {
          rfbLogPerror("rfbProcessClientNormalMessage: write");
          rfbCloseClient(cl);
          return;
        }
      }

      return;
    }  /* rfbSetEncodings */

    case rfbFramebufferUpdateRequest:
    {
      RegionRec tmpRegion;
      BoxRec box;

      READ(((char *)&msg) + 1, sz_rfbFramebufferUpdateRequestMsg - 1)

      box.x1 = Swap16IfLE(msg.fur.x);
      box.y1 = Swap16IfLE(msg.fur.y);
      box.x2 = box.x1 + Swap16IfLE(msg.fur.w);
      box.y2 = box.y1 + Swap16IfLE(msg.fur.h);
      SAFE_REGION_INIT(pScreen, &tmpRegion, &box, 0);

      if (!msg.fur.incremental || !cl->continuousUpdates)
        REGION_UNION(pScreen, &cl->requestedRegion, &cl->requestedRegion,
                     &tmpRegion);

      if (!cl->readyForSetColourMapEntries) {
        /* client hasn't sent a SetPixelFormat so is using server's */
        cl->readyForSetColourMapEntries = TRUE;
        if (!cl->format.trueColour) {
          if (!rfbSetClientColourMap(cl, 0, 0)) {
            REGION_UNINIT(pScreen, &tmpRegion);
            return;
          }
        }
      }

      if (!msg.fur.incremental) {
        REGION_UNION(pScreen, &cl->modifiedRegion, &cl->modifiedRegion,
                     &tmpRegion);
        REGION_SUBTRACT(pScreen, &cl->copyRegion, &cl->copyRegion, &tmpRegion);
        REGION_UNION(pScreen, &cl->ifRegion, &cl->ifRegion, &tmpRegion);
        cl->pendingExtDesktopResize = TRUE;
      }

      if (FB_UPDATE_PENDING(cl) &&
          (!cl->deferredUpdateScheduled || rfbDeferUpdateTime == 0 ||
           gettime() - cl->deferredUpdateStart >=
           (double)rfbDeferUpdateTime)) {
        if (rfbSendFramebufferUpdate(cl))
          cl->deferredUpdateScheduled = FALSE;
      }

      REGION_UNINIT(pScreen, &tmpRegion);
      return;
    }

    case rfbKeyEvent:

      cl->rfbKeyEventsRcvd++;

      READ(((char *)&msg) + 1, sz_rfbKeyEventMsg - 1)

      if (!rfbViewOnly && !cl->viewOnly)
        KeyEvent((KeySym)Swap32IfLE(msg.ke.key), msg.ke.down);

      return;

    case rfbPointerEvent:

      cl->rfbPointerEventsRcvd++;

      READ(((char *)&msg) + 1, sz_rfbPointerEventMsg - 1)

      if (pointerClient && (pointerClient != cl))
        return;

      if (msg.pe.buttonMask == 0)
        pointerClient = NULL;
      else
        pointerClient = cl;

      if (!rfbViewOnly && !cl->viewOnly) {
        cl->cursorX = (int)Swap16IfLE(msg.pe.x);
        cl->cursorY = (int)Swap16IfLE(msg.pe.y);
        PtrAddEvent(msg.pe.buttonMask, cl->cursorX, cl->cursorY, cl);
      }
      return;

    case rfbClientCutText:
    {
      int ignoredBytes = 0;

      READ(((char *)&msg) + 1, sz_rfbClientCutTextMsg - 1)

      msg.cct.length = Swap32IfLE(msg.cct.length);
      if (msg.cct.length > rfbMaxClipboard) {
        rfbLog("Truncating %d-byte clipboard update to %d bytes.\n",
               msg.cct.length, rfbMaxClipboard);
        ignoredBytes = msg.cct.length - rfbMaxClipboard;
        msg.cct.length = rfbMaxClipboard;
      }

      if (msg.cct.length <= 0) return;
      str = (char *)malloc(msg.cct.length);
      if (str == NULL) {
        rfbLogPerror("rfbProcessClientNormalMessage: rfbClientCutText out of memory");
        rfbCloseClient(cl);
        return;
      }

      if ((n = ReadExact(cl, str, msg.cct.length)) <= 0) {
        if (n != 0)
          rfbLogPerror("rfbProcessClientNormalMessage: read");
        free(str);
        rfbCloseClient(cl);
        return;
      }

      if (ignoredBytes > 0) {
        if ((n = SkipExact(cl, ignoredBytes)) <= 0) {
          if (n != 0)
            rfbLogPerror("rfbProcessClientNormalMessage: read");
          free(str);
          rfbCloseClient(cl);
          return;
        }
      }

      /* NOTE: We do not accept cut text from a view-only client */
      if (!rfbViewOnly && !cl->viewOnly && !rfbAuthDisableCBRecv) {
        vncClientCutText(str, msg.cct.length);
        if (rfbSyncCutBuffer) rfbSetXCutText(str, msg.cct.length);
      }

      free(str);
      return;
    }

    case rfbEnableContinuousUpdates:
    {
      BoxRec box;

      READ(((char *)&msg) + 1, sz_rfbEnableContinuousUpdatesMsg - 1)

      if (!cl->enableFence || !cl->enableCU) {
        rfbLog("Ignoring request to enable continuous updates because the client does not\n");
        rfbLog("support the flow control extensions.\n");
        return;
      }

      box.x1 = Swap16IfLE(msg.ecu.x);
      box.y1 = Swap16IfLE(msg.ecu.y);
      box.x2 = box.x1 + Swap16IfLE(msg.ecu.w);
      box.y2 = box.y1 + Swap16IfLE(msg.ecu.h);
      SAFE_REGION_INIT(pScreen, &cl->cuRegion, &box, 0);

      cl->continuousUpdates = msg.ecu.enable;
      if (cl->continuousUpdates) {
        REGION_EMPTY(pScreen, &cl->requestedRegion);
        if (!rfbSendFramebufferUpdate(cl))
          return;
      } else {
        if (!rfbSendEndOfCU(cl))
          return;
      }

      rfbLog("Continuous updates %s\n",
             cl->continuousUpdates ? "enabled" : "disabled");
      return;
    }

    case rfbFence:
    {
      CARD32 flags;
      char data[64];

      READ(((char *)&msg) + 1, sz_rfbFenceMsg - 1)

      flags = Swap32IfLE(msg.f.flags);

      READ(data, msg.f.length)

      if (msg.f.length > sizeof(data))
        rfbLog("Ignoring fence.  Payload of %d bytes is too large.\n",
               msg.f.length);
      else
        HandleFence(cl, flags, msg.f.length, data);
      return;
    }

    #define EDSERROR(format, args...) {  \
      if (!strlen(errMsg))  \
        snprintf(errMsg, 256, "Desktop resize ERROR: "format"\n", args);  \
      result = rfbEDSResultInvalid;  \
    }

    case rfbSetDesktopSize:
    {
      int i;
      struct xorg_list newScreens;
      rfbClientPtr cl2;
      int result = rfbEDSResultSuccess;
      char errMsg[256] = "\0";
      ScreenPtr pScreen = screenInfo.screens[0];

      READ(((char *)&msg) + 1, sz_rfbSetDesktopSizeMsg - 1)

      if (msg.sds.numScreens < 1)
        EDSERROR("Requested number of screens %d is invalid",
                 msg.sds.numScreens);

      msg.sds.w = Swap16IfLE(msg.sds.w);
      msg.sds.h = Swap16IfLE(msg.sds.h);
      if (msg.sds.w < 1 || msg.sds.h < 1)
        EDSERROR("Requested framebuffer dimensions %dx%d are invalid",
                 msg.sds.w, msg.sds.h);

      xorg_list_init(&newScreens);
      for (i = 0; i < msg.sds.numScreens; i++) {
        rfbScreenInfo *screen = rfbNewScreen(0, 0, 0, 0, 0, 0);

        READ((char *)&screen->s, sizeof(rfbScreenDesc))
        screen->s.id = Swap32IfLE(screen->s.id);
        screen->s.x = Swap16IfLE(screen->s.x);
        screen->s.y = Swap16IfLE(screen->s.y);
        screen->s.w = Swap16IfLE(screen->s.w);
        screen->s.h = Swap16IfLE(screen->s.h);
        screen->s.flags = Swap32IfLE(screen->s.flags);
        if (screen->s.w < 1 || screen->s.h < 1)
          EDSERROR("Screen 0x%.8x requested dimensions %dx%d are invalid",
                   (unsigned int)screen->s.id, screen->s.w, screen->s.h);
        if (screen->s.x >= msg.sds.w || screen->s.y >= msg.sds.h ||
            screen->s.x + screen->s.w > msg.sds.w ||
            screen->s.y + screen->s.h > msg.sds.h)
          EDSERROR("Screen 0x%.8x requested geometry %dx%d+%d+%d exceeds requested framebuffer dimensions",
                   (unsigned int)screen->s.id, screen->s.w, screen->s.h,
                   screen->s.x, screen->s.y);
        if (rfbFindScreenID(&newScreens, screen->s.id)) {
          EDSERROR("Screen 0x%.8x duplicate ID", (unsigned int)screen->s.id);
          free(screen);
        } else
          rfbAddScreen(&newScreens, screen);
      }

      if (cl->viewOnly) {
        rfbLog("NOTICE: Ignoring remote desktop resize request from a view-only client.\n");
        result = rfbEDSResultProhibited;
      } else if (result == rfbEDSResultSuccess) {
        result = ResizeDesktop(pScreen, cl, msg.sds.w, msg.sds.h, &newScreens);
        if (result == rfbEDSResultSuccess)
          return;
      } else
        rfbLog(errMsg);

      rfbRemoveScreens(&newScreens);

      /* Send back the error only to the requesting client.  This loop is
         necessary because the client may have been shut down as a result of
         an error in ResizeDesktop(). */
      for (cl2 = rfbClientHead; cl2; cl2 = cl2->next) {
        if (cl2 == cl) {
          cl2->pendingExtDesktopResize = TRUE;
          cl2->reason = rfbEDSReasonClient;
          cl2->result = result;
          rfbSendFramebufferUpdate(cl2);
          break;
        }
      }

      return;
    }

    case rfbGIIClient:
    {
      CARD8 endianAndSubType, littleEndian, subType;

      READ((char *)&endianAndSubType, 1);
      littleEndian = (endianAndSubType & rfbGIIBE) ? 0 : 1;
      subType = endianAndSubType & ~rfbGIIBE;

      switch (subType) {

        case rfbGIIVersion:

          READ((char *)&msg.giicv.length, sz_rfbGIIClientVersionMsg - 2);
          if (littleEndian != *(const char *)&rfbEndianTest) {
            msg.giicv.length = Swap16(msg.giicv.length);
            msg.giicv.version = Swap16(msg.giicv.version);
          }
          if (msg.giicv.length != sz_rfbGIIClientVersionMsg - 4 ||
              msg.giicv.version < 1) {
            rfbLog("ERROR: Malformed GII client version message\n");
            rfbCloseClient(cl);
            return;
          }
          rfbLog("Client supports GII version %d\n", msg.giicv.version);
          break;

        case rfbGIIDeviceCreate:
        {
          int i;
          rfbDevInfo dev;
          rfbGIIDeviceCreatedMsg dcmsg;

          memset(&dev, 0, sizeof(dev));
          dcmsg.deviceOrigin = 0;

          READ((char *)&msg.giidc.length, sz_rfbGIIDeviceCreateMsg - 2);
          if (littleEndian != *(const char *)&rfbEndianTest) {
            msg.giidc.length = Swap16(msg.giidc.length);
            msg.giidc.vendorID = Swap32(msg.giidc.vendorID);
            msg.giidc.productID = Swap32(msg.giidc.productID);
            msg.giidc.canGenerate = Swap32(msg.giidc.canGenerate);
            msg.giidc.numRegisters = Swap32(msg.giidc.numRegisters);
            msg.giidc.numValuators = Swap32(msg.giidc.numValuators);
            msg.giidc.numButtons = Swap32(msg.giidc.numButtons);
          }

          rfbLog("GII Device Create: %s\n", msg.giidc.deviceName);
#ifdef GII_DEBUG
          rfbLog("    Vendor ID: %d\n", msg.giidc.vendorID);
          rfbLog("    Product ID: %d\n", msg.giidc.productID);
          rfbLog("    Event mask: %.8x\n", msg.giidc.canGenerate);
          rfbLog("    Registers: %d\n", msg.giidc.numRegisters);
          rfbLog("    Valuators: %d\n", msg.giidc.numValuators);
          rfbLog("    Buttons: %d\n", msg.giidc.numButtons);
#endif

          if (msg.giidc.length != sz_rfbGIIDeviceCreateMsg - 4 +
              msg.giidc.numValuators * sz_rfbGIIValuator) {
            rfbLog("ERROR: Malformed GII device create message\n");
            rfbCloseClient(cl);
            return;
          }

          if (msg.giidc.numButtons > MAX_BUTTONS) {
            rfbLog("GII device create ERROR: %d buttons exceeds max of %d\n",
                   msg.giidc.numButtons, MAX_BUTTONS);
            SKIP(msg.giidc.numValuators * sz_rfbGIIValuator);
            goto sendMessage;
          }
          if (msg.giidc.numValuators > MAX_VALUATORS) {
            rfbLog("GII device create ERROR: %d valuators exceeds max of %d\n",
                   msg.giidc.numValuators, MAX_VALUATORS);
            SKIP(msg.giidc.numValuators * sz_rfbGIIValuator);
            goto sendMessage;
          }

          memcpy(&dev.name, msg.giidc.deviceName, 32);
          dev.numButtons = msg.giidc.numButtons;
          dev.numValuators = msg.giidc.numValuators;
          dev.eventMask = msg.giidc.canGenerate;
          dev.mode =
            (dev.eventMask & rfbGIIValuatorAbsoluteMask) ? Absolute : Relative;
          dev.productID = msg.giidc.productID;

          if (dev.mode == Relative) {
            rfbLog("GII device create ERROR: relative valuators not supported (yet)\n");
            SKIP(msg.giidc.numValuators * sz_rfbGIIValuator);
            goto sendMessage;
          }

          for (i = 0; i < dev.numValuators; i++) {
            rfbGIIValuator *v = &dev.valuators[i];
            READ((char *)v, sz_rfbGIIValuator);
            if (littleEndian != *(const char *)&rfbEndianTest) {
              v->index = Swap32(v->index);
              v->rangeMin = Swap32((CARD32)v->rangeMin);
              v->rangeCenter = Swap32((CARD32)v->rangeCenter);
              v->rangeMax = Swap32((CARD32)v->rangeMax);
              v->siUnit = Swap32(v->siUnit);
              v->siAdd = Swap32((CARD32)v->siAdd);
              v->siMul = Swap32((CARD32)v->siMul);
              v->siDiv = Swap32((CARD32)v->siDiv);
              v->siShift = Swap32((CARD32)v->siShift);
            }

#ifdef GII_DEBUG
            rfbLog("    Valuator: %s (%s)\n", v->longName, v->shortName);
            rfbLog("        Index: %d\n", v->index);
            rfbLog("        Range: min = %d, center = %d, max = %d\n",
                   v->rangeMin, v->rangeCenter, v->rangeMax);
            rfbLog("        SI unit: %d\n", v->siUnit);
            rfbLog("        SI add: %d\n", v->siAdd);
            rfbLog("        SI multiply: %d\n", v->siMul);
            rfbLog("        SI divide: %d\n", v->siDiv);
            rfbLog("        SI shift: %d\n", v->siShift);
#endif
          }

          for (i = 0; i < cl->numDevices; i++) {
            if (!strcmp(dev.name, cl->devices[i].name)) {
              rfbLog("Device \'%s\' already exists with GII device ID %d\n",
                     dev.name, i + 1);
              dcmsg.deviceOrigin = Swap32IfLE(i + 1);
              goto sendMessage;
            }
          }

          if (rfbVirtualTablet || AddExtInputDevice(&dev)) {
            memcpy(&cl->devices[cl->numDevices], &dev, sizeof(dev));
            cl->numDevices++;
            dcmsg.deviceOrigin = Swap32IfLE(cl->numDevices);
          }
          rfbLog("GII device ID = %d\n", cl->numDevices);

          sendMessage:
          /* Send back a GII device created message */
          dcmsg.type = rfbGIIServer;
          /* We always send as big endian to make things easier on the Java
             viewer. */
          dcmsg.endianAndSubType = rfbGIIDeviceCreate | rfbGIIBE;
          dcmsg.length = Swap16IfLE(sz_rfbGIIDeviceCreatedMsg - 4);

          if (WriteExact(cl, (char *)&dcmsg, sz_rfbGIIDeviceCreatedMsg) < 0) {
            rfbLogPerror("rfbProcessClientNormalMessage: write");
            rfbCloseClient(cl);
            return;
          }

          break;
        }

        case rfbGIIDeviceDestroy:

          READ((char *)&msg.giidd.length, sz_rfbGIIDeviceDestroyMsg - 2);
          if (littleEndian != *(const char *)&rfbEndianTest) {
            msg.giidd.length = Swap16(msg.giidd.length);
            msg.giidd.deviceOrigin = Swap32(msg.giidd.deviceOrigin);
          }
          if (msg.giidd.length != sz_rfbGIIDeviceDestroyMsg - 4) {
            rfbLog("ERROR: Malformed GII device create message\n");
            rfbCloseClient(cl);
            return;
          }

          RemoveExtInputDevice(cl, msg.giidd.deviceOrigin - 1);

          break;

        case rfbGIIEvent:
        {
          CARD16 length;

          READ((char *)&length, sizeof(CARD16));
          if (littleEndian != *(const char *)&rfbEndianTest)
            length = Swap16(length);

          while (length > 0) {
            CARD8 eventSize, eventType;

            READ((char *)&eventSize, 1);
            READ((char *)&eventType, 1);

            switch (eventType) {

              case rfbGIIButtonPress:
              case rfbGIIButtonRelease:
              {
                rfbGIIButtonEvent b;
                rfbDevInfo *dev;

                READ((char *)&b.pad, sz_rfbGIIButtonEvent - 2);
                if (littleEndian != *(const char *)&rfbEndianTest) {
                  b.deviceOrigin = Swap32(b.deviceOrigin);
                  b.buttonNumber = Swap32(b.buttonNumber);
                }
                if (eventSize != sz_rfbGIIButtonEvent || b.deviceOrigin <= 0 ||
                    b.buttonNumber < 1) {
                  rfbLog("ERROR: Malformed GII button event\n");
                  rfbCloseClient(cl);
                  return;
                }
                if (eventSize > length) {
                  rfbLog("ERROR: Malformed GII event message\n");
                  rfbCloseClient(cl);
                  return;
                }
                length -= eventSize;
                if (b.deviceOrigin < 1 || b.deviceOrigin > cl->numDevices) {
                  rfbLog("ERROR: GII button event from non-existent device %d\n",
                         b.deviceOrigin);
                  rfbCloseClient(cl);
                  return;
                }
                dev = &cl->devices[b.deviceOrigin - 1];
                if ((eventType == rfbGIIButtonPress &&
                     (dev->eventMask & rfbGIIButtonPressMask) == 0) ||
                    (eventType == rfbGIIButtonRelease &&
                     (dev->eventMask & rfbGIIButtonReleaseMask) == 0)) {
                  rfbLog("ERROR: Device %d can't generate GII button events\n",
                         b.deviceOrigin);
                  rfbCloseClient(cl);
                  return;
                }
                if (b.buttonNumber > dev->numButtons) {
                  rfbLog("ERROR: GII button %d event for device %d exceeds button count (%d)\n",
                         b.buttonNumber, b.deviceOrigin, dev->numButtons);
                  rfbCloseClient(cl);
                  return;
                }
#ifdef GII_DEBUG
                rfbLog("Device %d button %d %s\n", b.deviceOrigin,
                       b.buttonNumber,
                       eventType == rfbGIIButtonPress ? "PRESS" : "release");
                fflush(stderr);
#endif
                ExtInputAddEvent(dev, eventType == rfbGIIButtonPress ?
                                 ButtonPress : ButtonRelease, b.buttonNumber);
                break;
              }

              case rfbGIIValuatorRelative:
              case rfbGIIValuatorAbsolute:
              {
                rfbGIIValuatorEvent v;
                int i;
                rfbDevInfo *dev;

                READ((char *)&v.pad, sz_rfbGIIValuatorEvent - 2);
                if (littleEndian != *(const char *)&rfbEndianTest) {
                  v.deviceOrigin = Swap32(v.deviceOrigin);
                  v.first = Swap32(v.first);
                  v.count = Swap32(v.count);
                }
                if (eventSize !=
                    sz_rfbGIIValuatorEvent + sizeof(int) * v.count) {
                  rfbLog("ERROR: Malformed GII valuator event\n");
                  rfbCloseClient(cl);
                  return;
                }
                if (eventSize > length) {
                  rfbLog("ERROR: Malformed GII event message\n");
                  rfbCloseClient(cl);
                  return;
                }
                length -= eventSize;
                if (v.deviceOrigin < 1 || v.deviceOrigin > cl->numDevices) {
                  rfbLog("ERROR: GII valuator event from non-existent device %d\n",
                         v.deviceOrigin);
                  rfbCloseClient(cl);
                  return;
                }
                dev = &cl->devices[v.deviceOrigin - 1];
                if ((eventType == rfbGIIValuatorRelative &&
                     (dev->eventMask & rfbGIIValuatorRelativeMask) == 0) ||
                    (eventType == rfbGIIValuatorAbsolute &&
                     (dev->eventMask & rfbGIIValuatorAbsoluteMask) == 0)) {
                  rfbLog("ERROR: Device %d cannot generate GII valuator events\n",
                         v.deviceOrigin);
                  rfbCloseClient(cl);
                  return;
                }
                if (v.first + v.count > dev->numValuators) {
                  rfbLog("ERROR: GII valuator event for device %d exceeds valuator count (%d)\n",
                         v.deviceOrigin, dev->numValuators);
                  rfbCloseClient(cl);
                  return;
                }
#ifdef GII_DEBUG
                rfbLog("Device %d Valuator %s first=%d count=%d:\n",
                       v.deviceOrigin,
                       eventType == rfbGIIValuatorRelative ? "rel" : "ABS",
                       v.first, v.count);
#endif
                for (i = v.first; i < v.first + v.count; i++) {
                  READ((char *)&dev->values[i], sizeof(int));
                  if (littleEndian != *(const char *)&rfbEndianTest)
                    dev->values[i] = Swap32((CARD32)dev->values[i]);
#ifdef GII_DEBUG
                  fprintf(stderr, "v[%d]=%d ", i, dev->values[i]);
#endif
                }
#ifdef GII_DEBUG
                fprintf(stderr, "\n");
#endif
                if (v.count > 0) {
                  dev->valFirst = v.first;
                  dev->valCount = v.count;
                  dev->mode = eventType == rfbGIIValuatorAbsolute ?
                              Absolute : Relative;
                  ExtInputAddEvent(dev, MotionNotify, 0);
                }
                break;
              }
              default:
                rfbLog("ERROR: This server cannot handle GII event type %d\n",
                       eventType);
                rfbCloseClient(cl);
                return;
            }  /* switch (eventType) */
          }  /* while (length > 0) */
          if (length != 0) {
            rfbLog("ERROR: Malformed GII event message\n");
            rfbCloseClient(cl);
            return;
          }
          break;
        }  /* rfbGIIEvent */
      }  /* switch (subType) */
      return;
    }  /* rfbGIIClient */

    default:

      rfbLog("rfbProcessClientNormalMessage: unknown message type %d\n",
             msg.type);
      rfbLog(" ... closing connection\n");
      rfbCloseClient(cl);
      return;
  }  /* switch (msg.type) */
}