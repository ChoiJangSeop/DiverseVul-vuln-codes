ProcXkbSetNames(ClientPtr client)
{
    DeviceIntPtr dev;
    CARD32 *tmp;
    Atom bad;
    int rc;

    REQUEST(xkbSetNamesReq);
    REQUEST_AT_LEAST_SIZE(xkbSetNamesReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_MASK_LEGAL(0x01, stuff->which, XkbAllNamesMask);

    /* check device-independent stuff */
    tmp = (CARD32 *) &stuff[1];

    if (stuff->which & XkbKeycodesNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbGeometryNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbSymbolsNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbPhysSymbolsNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbTypesNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbCompatNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }

    /* start of device-dependent tests */
    rc = _XkbSetNamesCheck(client, dev, stuff, tmp);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {

                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetNamesCheck(client, other, stuff, tmp);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    /* everything is okay -- update names */

    rc = _XkbSetNames(client, dev, stuff);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {

                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success)
                    _XkbSetNames(client, other, stuff);
            }
        }
    }

    /* everything is okay -- update names */

    return Success;
}