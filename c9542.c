SetDeviceIndicators(char *wire,
                    DeviceIntPtr dev,
                    unsigned changed,
                    int num,
                    int *status_rtrn,
                    ClientPtr client, xkbExtensionDeviceNotify * ev)
{
    xkbDeviceLedsWireDesc *ledWire;
    int i;
    XkbEventCauseRec cause;
    unsigned namec, mapc, statec;
    xkbExtensionDeviceNotify ed;
    XkbChangesRec changes;
    DeviceIntPtr kbd;

    memset((char *) &ed, 0, sizeof(xkbExtensionDeviceNotify));
    memset((char *) &changes, 0, sizeof(XkbChangesRec));
    XkbSetCauseXkbReq(&cause, X_kbSetDeviceInfo, client);
    ledWire = (xkbDeviceLedsWireDesc *) wire;
    for (i = 0; i < num; i++) {
        register int n;
        register unsigned bit;
        CARD32 *atomWire;
        xkbIndicatorMapWireDesc *mapWire;
        XkbSrvLedInfoPtr sli;

        namec = mapc = statec = 0;
        sli = XkbFindSrvLedInfo(dev, ledWire->ledClass, ledWire->ledID,
                                XkbXI_IndicatorMapsMask);
        if (!sli) {
            /* SHOULD NEVER HAPPEN!! */
            return (char *) ledWire;
        }

        atomWire = (CARD32 *) &ledWire[1];
        if (changed & XkbXI_IndicatorNamesMask) {
            namec = sli->namesPresent | ledWire->namesPresent;
            memset((char *) sli->names, 0, XkbNumIndicators * sizeof(Atom));
        }
        if (ledWire->namesPresent) {
            sli->namesPresent = ledWire->namesPresent;
            memset((char *) sli->names, 0, XkbNumIndicators * sizeof(Atom));
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->namesPresent & bit) {
                    sli->names[n] = (Atom) *atomWire;
                    if (sli->names[n] == None)
                        ledWire->namesPresent &= ~bit;
                    atomWire++;
                }
            }
        }
        mapWire = (xkbIndicatorMapWireDesc *) atomWire;
        if (changed & XkbXI_IndicatorMapsMask) {
            mapc = sli->mapsPresent | ledWire->mapsPresent;
            sli->mapsPresent = ledWire->mapsPresent;
            memset((char *) sli->maps, 0,
                   XkbNumIndicators * sizeof(XkbIndicatorMapRec));
        }
        if (ledWire->mapsPresent) {
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->mapsPresent & bit) {
                    sli->maps[n].flags = mapWire->flags;
                    sli->maps[n].which_groups = mapWire->whichGroups;
                    sli->maps[n].groups = mapWire->groups;
                    sli->maps[n].which_mods = mapWire->whichMods;
                    sli->maps[n].mods.mask = mapWire->mods;
                    sli->maps[n].mods.real_mods = mapWire->realMods;
                    sli->maps[n].mods.vmods = mapWire->virtualMods;
                    sli->maps[n].ctrls = mapWire->ctrls;
                    mapWire++;
                }
            }
        }
        if (changed & XkbXI_IndicatorStateMask) {
            statec = sli->effectiveState ^ ledWire->state;
            sli->explicitState &= ~statec;
            sli->explicitState |= (ledWire->state & statec);
        }
        if (namec)
            XkbApplyLedNameChanges(dev, sli, namec, &ed, &changes, &cause);
        if (mapc)
            XkbApplyLedMapChanges(dev, sli, mapc, &ed, &changes, &cause);
        if (statec)
            XkbApplyLedStateChanges(dev, sli, statec, &ed, &changes, &cause);

        kbd = dev;
        if ((sli->flags & XkbSLI_HasOwnState) == 0)
            kbd = inputInfo.keyboard;

        XkbFlushLedEvents(dev, kbd, sli, &ed, &changes, &cause);
        ledWire = (xkbDeviceLedsWireDesc *) mapWire;
    }
    return (char *) ledWire;
}