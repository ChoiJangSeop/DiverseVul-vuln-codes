_XkbSetNamesCheck(ClientPtr client, DeviceIntPtr dev,
                  xkbSetNamesReq * stuff, CARD32 *data)
{
    XkbDescRec *xkb;
    CARD32 *tmp;
    Atom bad = None;

    tmp = data;
    xkb = dev->key->xkbInfo->desc;

    if (stuff->which & XkbKeyTypeNamesMask) {
        int i;
        CARD32 *old;

        if (stuff->nTypes < 1) {
            client->errorValue = _XkbErrCode2(0x02, stuff->nTypes);
            return BadValue;
        }
        if ((unsigned) (stuff->firstType + stuff->nTypes - 1) >=
            xkb->map->num_types) {
            client->errorValue =
                _XkbErrCode4(0x03, stuff->firstType, stuff->nTypes,
                             xkb->map->num_types);
            return BadValue;
        }
        if (((unsigned) stuff->firstType) <= XkbLastRequiredType) {
            client->errorValue = _XkbErrCode2(0x04, stuff->firstType);
            return BadAccess;
        }
        old = tmp;
        tmp = _XkbCheckAtoms(tmp, stuff->nTypes, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
        for (i = 0; i < stuff->nTypes; i++, old++) {
            if (!_XkbCheckTypeName((Atom) *old, stuff->firstType + i))
                client->errorValue = _XkbErrCode2(0x05, i);
        }
    }
    if (stuff->which & XkbKTLevelNamesMask) {
        unsigned i;
        XkbKeyTypePtr type;
        CARD8 *width;

        if (stuff->nKTLevels < 1) {
            client->errorValue = _XkbErrCode2(0x05, stuff->nKTLevels);
            return BadValue;
        }
        if ((unsigned) (stuff->firstKTLevel + stuff->nKTLevels - 1) >=
            xkb->map->num_types) {
            client->errorValue = _XkbErrCode4(0x06, stuff->firstKTLevel,
                                              stuff->nKTLevels,
                                              xkb->map->num_types);
            return BadValue;
        }
        width = (CARD8 *) tmp;
        tmp = (CARD32 *) (((char *) tmp) + XkbPaddedSize(stuff->nKTLevels));
        type = &xkb->map->types[stuff->firstKTLevel];
        for (i = 0; i < stuff->nKTLevels; i++, type++) {
            if (width[i] == 0)
                continue;
            else if (width[i] != type->num_levels) {
                client->errorValue = _XkbErrCode4(0x07, i + stuff->firstKTLevel,
                                                  type->num_levels, width[i]);
                return BadMatch;
            }
            tmp = _XkbCheckAtoms(tmp, width[i], client->swapped, &bad);
            if (!tmp) {
                client->errorValue = bad;
                return BadAtom;
            }
        }
    }
    if (stuff->which & XkbIndicatorNamesMask) {
        if (stuff->indicators == 0) {
            client->errorValue = 0x08;
            return BadMatch;
        }
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumIndicators, stuff->indicators,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbVirtualModNamesMask) {
        if (stuff->virtualMods == 0) {
            client->errorValue = 0x09;
            return BadMatch;
        }
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumVirtualMods,
                                   (CARD32) stuff->virtualMods,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbGroupNamesMask) {
        if (stuff->groupNames == 0) {
            client->errorValue = 0x0a;
            return BadMatch;
        }
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumKbdGroups,
                                   (CARD32) stuff->groupNames,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbKeyNamesMask) {
        if (stuff->firstKey < (unsigned) xkb->min_key_code) {
            client->errorValue = _XkbErrCode3(0x0b, xkb->min_key_code,
                                              stuff->firstKey);
            return BadValue;
        }
        if (((unsigned) (stuff->firstKey + stuff->nKeys - 1) >
             xkb->max_key_code) || (stuff->nKeys < 1)) {
            client->errorValue =
                _XkbErrCode4(0x0c, xkb->max_key_code, stuff->firstKey,
                             stuff->nKeys);
            return BadValue;
        }
        tmp += stuff->nKeys;
    }
    if ((stuff->which & XkbKeyAliasesMask) && (stuff->nKeyAliases > 0)) {
        tmp += stuff->nKeyAliases * 2;
    }
    if (stuff->which & XkbRGNamesMask) {
        if (stuff->nRadioGroups < 1) {
            client->errorValue = _XkbErrCode2(0x0d, stuff->nRadioGroups);
            return BadValue;
        }
        tmp = _XkbCheckAtoms(tmp, stuff->nRadioGroups, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if ((tmp - ((CARD32 *) stuff)) != stuff->length) {
        client->errorValue = stuff->length;
        return BadLength;
    }

    return Success;
}