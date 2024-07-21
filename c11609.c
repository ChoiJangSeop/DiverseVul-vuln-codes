parseUserInfo(DUL_USERINFO * userInfo,
              unsigned char *buf,
              unsigned long *itemLength,
              unsigned char typeRQorAC,
              unsigned long availData /* bytes left for in this PDU */)
{
    unsigned short userLength;
    unsigned long length;
    OFCondition cond = EC_Normal;
    PRV_SCUSCPROLE *role;
    SOPClassExtendedNegotiationSubItem *extNeg = NULL;
    UserIdentityNegotiationSubItem *usrIdent = NULL;

    // minimum allowed size is 4 byte (case where the length of the user data is 0),
    // else we read past the buffer end
    if (availData < 4)
        return makeLengthError("user info", availData, 4);

    // skip item type (50H) field
    userInfo->type = *buf++;
    // skip unused ("reserved") field
    userInfo->rsv1 = *buf++;
    // get and remember announced length of user data
    EXTRACT_SHORT_BIG(buf, userInfo->length);
    // .. and skip over the two length field bytes
    buf += 2;

    // userLength contains announced length of full user item structure,
    // will be used here to count down the available data later
    userLength = userInfo->length;
    // itemLength contains full length of the user item including the 4 bytes extra header (type, reserved + 2 for length)
    *itemLength = userLength + 4;

    // does this item claim to be larger than the available data?
    if (availData < *itemLength)
        return makeLengthError("user info", availData, 0, userLength);

    DCMNET_TRACE("Parsing user info field ("
            << STD_NAMESPACE hex << STD_NAMESPACE setfill('0') << STD_NAMESPACE setw(2) << (unsigned int)userInfo->type
            << STD_NAMESPACE dec << "), Length: " << (unsigned long)userInfo->length);
    // parse through different types of user items as long as we have data
    while (userLength > 0) {
        DCMNET_TRACE("Parsing remaining " << (long)userLength << " bytes of User Information" << OFendl
                << "Next item type: "
                << STD_NAMESPACE hex << STD_NAMESPACE setfill('0') << STD_NAMESPACE setw(2) << (unsigned int)*buf);
        switch (*buf) {
        case DUL_TYPEMAXLENGTH:
            cond = parseMaxPDU(&userInfo->maxLength, buf, &length, userLength);
            if (cond.bad())
                return cond;
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("maximum length sub-item", userLength, length);
            DCMNET_TRACE("Successfully parsed Maximum PDU Length");
            break;
        case DUL_TYPEIMPLEMENTATIONCLASSUID:
            cond = parseSubItem(&userInfo->implementationClassUID,
                                buf, &length, userLength);
            if (cond.bad())
                return cond;
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("Implementation Class UID sub-item", userLength, length);
            break;

        case DUL_TYPEASYNCOPERATIONS:
            cond = parseDummy(buf, &length, userLength);
            if (cond.bad())
                return cond;
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("asynchronous operation user item type", userLength, length);
            break;
        case DUL_TYPESCUSCPROLE:
            role = (PRV_SCUSCPROLE*)malloc(sizeof(PRV_SCUSCPROLE));
            if (role == NULL) return EC_MemoryExhausted;
            cond = parseSCUSCPRole(role, buf, &length, userLength);
            if (cond.bad()) return cond;
            LST_Enqueue(&userInfo->SCUSCPRoleList, (LST_NODE*)role);
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("SCP/SCU Role Selection sub-item", userLength, length);
            break;
        case DUL_TYPEIMPLEMENTATIONVERSIONNAME:
            cond = parseSubItem(&userInfo->implementationVersionName,
                                buf, &length, userLength);
            if (cond.bad()) return cond;
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("Implementation Version Name structure", userLength, length);
            break;

        case DUL_TYPESOPCLASSEXTENDEDNEGOTIATION:
            /* parse an extended negotiation sub-item */
            extNeg = new SOPClassExtendedNegotiationSubItem;
            if (extNeg == NULL)  return EC_MemoryExhausted;
            cond = parseExtNeg(extNeg, buf, &length, userLength);
            if (cond.bad()) return cond;
            if (userInfo->extNegList == NULL)
            {
                userInfo->extNegList = new SOPClassExtendedNegotiationSubItemList;
                if (userInfo->extNegList == NULL)  return EC_MemoryExhausted;
            }
            userInfo->extNegList->push_back(extNeg);
            buf += length;
            if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
              return makeLengthError("SOP Class Extended Negotiation sub-item", userLength, length);
            break;

        case DUL_TYPENEGOTIATIONOFUSERIDENTITY_REQ:
        case DUL_TYPENEGOTIATIONOFUSERIDENTITY_ACK:
          if (typeRQorAC == DUL_TYPEASSOCIATERQ)
            usrIdent = new UserIdentityNegotiationSubItemRQ();
          else // assume DUL_TYPEASSOCIATEAC
            usrIdent = new UserIdentityNegotiationSubItemAC();
          if (usrIdent == NULL) return EC_MemoryExhausted;
          cond = usrIdent->parseFromBuffer(buf, length /*return value*/, userLength);
          if (cond.bad())
          {
            delete usrIdent;
            return cond;
          }
          userInfo->usrIdent = usrIdent;
          buf += length;
          if (!OFStandard::safeSubtract(userLength, OFstatic_cast(short unsigned int, length), userLength))
            return makeLengthError("User Identity sub-item", userLength, length);
          break;
        default:
            // we hit an unknown user item that is not defined in the standard
            // or still unknown to DCMTK
            cond = parseDummy(buf, &length /* returns bytes "handled" by parseDummy */, userLength /* data available in bytes for user item */);
            if (cond.bad())
              return cond;
            // skip the bytes read
            buf += length;
            // subtract bytes of parsed data from available data bytes
            if (OFstatic_cast(unsigned short, length) != length
                || !OFStandard::safeSubtract(userLength, OFstatic_cast(unsigned short, length), userLength))
              return makeUnderflowError("unknown user item", userLength, length);
            break;
        }
    }

    return EC_Normal;
}