CheckSetDeviceIndicators(char *wire,
                         DeviceIntPtr dev,
                         int num, int *status_rtrn, ClientPtr client)
{
    xkbDeviceLedsWireDesc *ledWire;
    int i;
    XkbSrvLedInfoPtr sli;

    ledWire = (xkbDeviceLedsWireDesc *) wire;
    for (i = 0; i < num; i++) {
        if (client->swapped) {
            swaps(&ledWire->ledClass);
            swaps(&ledWire->ledID);
            swapl(&ledWire->namesPresent);
            swapl(&ledWire->mapsPresent);
            swapl(&ledWire->physIndicators);
        }

        sli = XkbFindSrvLedInfo(dev, ledWire->ledClass, ledWire->ledID,
                                XkbXI_IndicatorsMask);
        if (sli != NULL) {
            register int n;
            register unsigned bit;
            int nMaps, nNames;
            CARD32 *atomWire;
            xkbIndicatorMapWireDesc *mapWire;

            nMaps = nNames = 0;
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->namesPresent & bit)
                    nNames++;
                if (ledWire->mapsPresent & bit)
                    nMaps++;
            }
            atomWire = (CARD32 *) &ledWire[1];
            if (nNames > 0) {
                for (n = 0; n < nNames; n++) {
                    if (client->swapped) {
                        swapl(atomWire);
                    }
                    CHK_ATOM_OR_NONE3(((Atom) (*atomWire)), client->errorValue,
                                      *status_rtrn, NULL);
                    atomWire++;
                }
            }
            mapWire = (xkbIndicatorMapWireDesc *) atomWire;
            if (nMaps > 0) {
                for (n = 0; n < nMaps; n++) {
                    if (client->swapped) {
                        swaps(&mapWire->virtualMods);
                        swapl(&mapWire->ctrls);
                    }
                    CHK_MASK_LEGAL3(0x21, mapWire->whichGroups,
                                    XkbIM_UseAnyGroup,
                                    client->errorValue, *status_rtrn, NULL);
                    CHK_MASK_LEGAL3(0x22, mapWire->whichMods, XkbIM_UseAnyMods,
                                    client->errorValue, *status_rtrn, NULL);
                    mapWire++;
                }
            }
            ledWire = (xkbDeviceLedsWireDesc *) mapWire;
        }
        else {
            /* SHOULD NEVER HAPPEN */
            return (char *) ledWire;
        }
    }
    return (char *) ledWire;
}