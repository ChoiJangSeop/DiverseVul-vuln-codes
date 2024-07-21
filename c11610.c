parseAssociate(unsigned char *buf, unsigned long pduLength,
               PRV_ASSOCIATEPDU * assoc)
{
    OFCondition cond = EC_Normal;
    unsigned char
        type;
    unsigned long
        itemLength;
    PRV_PRESENTATIONCONTEXTITEM
        * context;

    (void) memset(assoc, 0, sizeof(*assoc));
    if ((assoc->presentationContextList = LST_Create()) == NULL) return EC_MemoryExhausted;
    if ((assoc->userInfo.SCUSCPRoleList = LST_Create()) == NULL) return EC_MemoryExhausted;

    // Check if the PDU actually is long enough for the fields we read
    if (pduLength < 2 + 2 + 16 + 16 + 32)
        return makeLengthError("associate PDU", pduLength, 2 + 2 + 16 + 16 + 32);

    assoc->type = *buf++;
    assoc->rsv1 = *buf++;
    EXTRACT_LONG_BIG(buf, assoc->length);
    buf += 4;

    EXTRACT_SHORT_BIG(buf, assoc->protocol);
    buf += 2;
    pduLength -= 2;
    if ((assoc->protocol & DUL_PROTOCOL) == 0)
    {
        char buffer[256];
        sprintf(buffer, "DUL Unsupported peer protocol %04x; expected %04x in %s", assoc->protocol, DUL_PROTOCOL, "parseAssociate");
        return makeDcmnetCondition(DULC_UNSUPPORTEDPEERPROTOCOL, OF_error, buffer);
    }
    assoc->rsv2[0] = *buf++;
    pduLength--;
    assoc->rsv2[1] = *buf++;
    pduLength--;
    (void) strncpy(assoc->calledAPTitle, (char *) buf, 16);
    assoc->calledAPTitle[16] = '\0';
    trim_trailing_spaces(assoc->calledAPTitle);

    buf += 16;
    pduLength -= 16;
    (void) strncpy(assoc->callingAPTitle, (char *) buf, 16);
    assoc->callingAPTitle[16] = '\0';
    trim_trailing_spaces(assoc->callingAPTitle);
    buf += 16;
    pduLength -= 16;
    (void) memcpy(assoc->rsv3, buf, 32);
    buf += 32;
    pduLength -= 32;

    if (DCM_dcmnetLogger.isEnabledFor(OFLogger::DEBUG_LOG_LEVEL)) {
        const char *s;
        DCMNET_DEBUG("Parsing an A-ASSOCIATE PDU");
        if (assoc->type == DUL_TYPEASSOCIATERQ)
            s = "A-ASSOCIATE RQ";
        else if (assoc->type == DUL_TYPEASSOCIATEAC)
            s = "A-ASSOCIATE AC";
        else
            s = "Unknown: Programming bug in parseAssociate";

/*      If we hit the "Unknown type", there is a programming bug somewhere.
**      This function is only supposed to parse A-ASSOCIATE PDUs and
**      expects its input to have been properly screened.
*/
        DCMNET_TRACE("PDU type: "
            << STD_NAMESPACE hex << ((unsigned int)assoc->type)
            << STD_NAMESPACE dec << " (" << s << "), PDU Length: " << assoc->length << OFendl
            << "DICOM Protocol: "
            << STD_NAMESPACE hex << assoc->protocol
            << STD_NAMESPACE dec << OFendl
            << "Called AP Title:  " << assoc->calledAPTitle << OFendl
            << "Calling AP Title: " << assoc->callingAPTitle);
    }
    while ((cond.good()) && (pduLength > 0))
    {
        type = *buf;
        DCMNET_TRACE("Parsing remaining " << pduLength << " bytes of A-ASSOCIATE PDU" << OFendl
                << "Next item type: "
                << STD_NAMESPACE hex << STD_NAMESPACE setfill('0') << STD_NAMESPACE setw(2) << ((unsigned int)type));
        switch (type) {
        case DUL_TYPEAPPLICATIONCONTEXT:
            cond = parseSubItem(&assoc->applicationContext,
                                buf, &itemLength, pduLength);
            if (cond.good())
            {
                buf += itemLength;
                if (!OFStandard::safeSubtract(pduLength, itemLength, pduLength))
                  return makeUnderflowError("Application Context item", pduLength, itemLength);
                DCMNET_TRACE("Successfully parsed Application Context");
            }
            break;
        case DUL_TYPEPRESENTATIONCONTEXTRQ:
        case DUL_TYPEPRESENTATIONCONTEXTAC:
            context = (PRV_PRESENTATIONCONTEXTITEM*)malloc(sizeof(PRV_PRESENTATIONCONTEXTITEM));
            if (context == NULL) return EC_MemoryExhausted;
            (void) memset(context, 0, sizeof(*context));
            cond = parsePresentationContext(type, context, buf, &itemLength, pduLength);
            if (cond.bad()) return cond;
            buf += itemLength;
            if (!OFStandard::safeSubtract(pduLength, itemLength, pduLength))
              return makeUnderflowError("Presentation Context item", pduLength, itemLength);
            LST_Enqueue(&assoc->presentationContextList, (LST_NODE*)context);
            DCMNET_TRACE("Successfully parsed Presentation Context");
            break;
        case DUL_TYPEUSERINFO:
            // parse user info, which can contain several sub-items like User
            // Identity Negotiation or SOP Class Extended Negotiation
            cond = parseUserInfo(&assoc->userInfo, buf, &itemLength, assoc->type, pduLength);
            if (cond.bad())
                return cond;
            buf += itemLength;
            if (!OFStandard::safeSubtract(pduLength, itemLength, pduLength))
              return makeUnderflowError("User Information item", pduLength, itemLength);
            DCMNET_TRACE("Successfully parsed User Information");
            break;
        default:
            cond = parseDummy(buf, &itemLength, pduLength);
            if (cond.bad())
                return cond;
            buf += itemLength;
            if (!OFStandard::safeSubtract(pduLength, itemLength, pduLength))
              return makeUnderflowError("unknown item type", pduLength, itemLength);
            break;
        }
    }
    return cond;
}