_XkbSetDeviceInfoCheck(ClientPtr client, DeviceIntPtr dev,
                       xkbSetDeviceInfoReq * stuff)
{
    char *wire;
    xkbExtensionDeviceNotify ed;

    memset((char *) &ed, 0, SIZEOF(xkbExtensionDeviceNotify));
    ed.deviceID = dev->id;
    wire = (char *) &stuff[1];
    if (stuff->change & XkbXI_ButtonActionsMask) {
        int nBtns, sz, i;
        XkbAction *acts;
        DeviceIntPtr kbd;

        nBtns = dev->button->numButtons;
        acts = dev->button->xkb_acts;
        if (acts == NULL) {
            acts = calloc(nBtns, sizeof(XkbAction));
            if (!acts)
                return BadAlloc;
            dev->button->xkb_acts = acts;
        }
        sz = stuff->nBtns * SIZEOF(xkbActionWireDesc);
        memcpy((char *) &acts[stuff->firstBtn], (char *) wire, sz);
        wire += sz;
        ed.reason |= XkbXI_ButtonActionsMask;
        ed.firstBtn = stuff->firstBtn;
        ed.nBtns = stuff->nBtns;

        if (dev->key)
            kbd = dev;
        else
            kbd = inputInfo.keyboard;
        acts = &dev->button->xkb_acts[stuff->firstBtn];
        for (i = 0; i < stuff->nBtns; i++, acts++) {
            if (acts->type != XkbSA_NoAction)
                XkbSetActionKeyMods(kbd->key->xkbInfo->desc, acts, 0);
        }
    }
    if (stuff->change & XkbXI_IndicatorsMask) {
        int status = Success;

        wire = SetDeviceIndicators(wire, dev, stuff->change,
                                   stuff->nDeviceLedFBs, &status, client, &ed);
        if (status != Success)
            return status;
    }
    if ((stuff->change) && (ed.reason))
        XkbSendExtensionDeviceNotify(dev, client, &ed);
    return Success;
}