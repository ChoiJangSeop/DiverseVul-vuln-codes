_XkbSetDeviceInfo(ClientPtr client, DeviceIntPtr dev,
                  xkbSetDeviceInfoReq * stuff)
{
    char *wire;

    wire = (char *) &stuff[1];
    if (stuff->change & XkbXI_ButtonActionsMask) {
        if (!dev->button) {
            client->errorValue = _XkbErrCode2(XkbErr_BadClass, ButtonClass);
            return XkbKeyboardErrorCode;
        }
        if ((stuff->firstBtn + stuff->nBtns) > dev->button->numButtons) {
            client->errorValue =
                _XkbErrCode4(0x02, stuff->firstBtn, stuff->nBtns,
                             dev->button->numButtons);
            return BadMatch;
        }
        wire += (stuff->nBtns * SIZEOF(xkbActionWireDesc));
    }
    if (stuff->change & XkbXI_IndicatorsMask) {
        int status = Success;

        wire = CheckSetDeviceIndicators(wire, dev, stuff->nDeviceLedFBs,
                                        &status, client);
        if (status != Success)
            return status;
    }
    if (((wire - ((char *) stuff)) / 4) != stuff->length)
        return BadLength;

    return Success;
}