cmsBool CMSEXPORT cmsWriteTag(cmsHPROFILE hProfile, cmsTagSignature sig, const void* data)
{
    _cmsICCPROFILE* Icc = (_cmsICCPROFILE*) hProfile;
    cmsTagTypeHandler* TypeHandler = NULL;
    cmsTagDescriptor* TagDescriptor = NULL;
    cmsTagTypeSignature Type;
    int i;
    cmsFloat64Number Version;
    char TypeString[5], SigString[5];


    if (data == NULL) {

         i = _cmsSearchTag(Icc, sig, FALSE);
         if (i >= 0)
             Icc ->TagNames[i] = (cmsTagSignature) 0;
         // Unsupported by now, reserved for future ampliations (delete)
         return FALSE;
    }

    i = _cmsSearchTag(Icc, sig, FALSE);
    if (i >=0) {

        if (Icc -> TagPtrs[i] != NULL) {

            // Already exists. Free previous version
            if (Icc ->TagSaveAsRaw[i]) {
                _cmsFree(Icc ->ContextID, Icc ->TagPtrs[i]);
            }
            else {
                TypeHandler = Icc ->TagTypeHandlers[i];

                if (TypeHandler != NULL) {

                    TypeHandler ->ContextID = Icc ->ContextID;              // As an additional parameter
                    TypeHandler ->ICCVersion = Icc ->Version;
                    TypeHandler->FreePtr(TypeHandler, Icc -> TagPtrs[i]);
                }
            }
        }
    }
    else  {
        // New one
        i = Icc -> TagCount;

        if (i >= MAX_TABLE_TAG) {
            cmsSignalError(Icc ->ContextID, cmsERROR_RANGE, "Too many tags (%d)", MAX_TABLE_TAG);
            return FALSE;
        }

        Icc -> TagCount++;
    }

    // This is not raw
    Icc ->TagSaveAsRaw[i] = FALSE;

    // This is not a link
    Icc ->TagLinked[i] = (cmsTagSignature) 0;

    // Get information about the TAG.
    TagDescriptor = _cmsGetTagDescriptor(sig);
    if (TagDescriptor == NULL){
         cmsSignalError(Icc ->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported tag '%x'", sig);
        return FALSE;
    }


    // Now we need to know which type to use. It depends on the version.
    Version = cmsGetProfileVersion(hProfile);

    if (TagDescriptor ->DecideType != NULL) {

        // Let the tag descriptor to decide the type base on depending on
        // the data. This is useful for example on parametric curves, where
        // curves specified by a table cannot be saved as parametric and needs
        // to be revented to single v2-curves, even on v4 profiles.

        Type = TagDescriptor ->DecideType(Version, data);
    }
    else {


        Type = TagDescriptor ->SupportedTypes[0];
    }

    // Does the tag support this type?
    if (!IsTypeSupported(TagDescriptor, Type)) {

        _cmsTagSignature2String(TypeString, (cmsTagSignature) Type);
        _cmsTagSignature2String(SigString,  sig);

        cmsSignalError(Icc ->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported type '%s' for tag '%s'", TypeString, SigString);
        return FALSE;
    }

    // Does we have a handler for this type?
    TypeHandler =  _cmsGetTagTypeHandler(Type);
    if (TypeHandler == NULL) {

        _cmsTagSignature2String(TypeString, (cmsTagSignature) Type);
        _cmsTagSignature2String(SigString,  sig);

        cmsSignalError(Icc ->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported type '%s' for tag '%s'", TypeString, SigString);
        return FALSE;           // Should never happen
    }


    // Fill fields on icc structure
    Icc ->TagTypeHandlers[i]  = TypeHandler;
    Icc ->TagNames[i]         = sig;
    Icc ->TagSizes[i]         = 0;
    Icc ->TagOffsets[i]       = 0;

    TypeHandler ->ContextID  = Icc ->ContextID;
    TypeHandler ->ICCVersion = Icc ->Version;
    Icc ->TagPtrs[i]         = TypeHandler ->DupPtr(TypeHandler, data, TagDescriptor ->ElemCount);

    if (Icc ->TagPtrs[i] == NULL)  {

        _cmsTagSignature2String(TypeString, (cmsTagSignature) Type);
        _cmsTagSignature2String(SigString,  sig);
        cmsSignalError(Icc ->ContextID, cmsERROR_CORRUPTION_DETECTED, "Malformed struct in type '%s' for tag '%s'", TypeString, SigString);

        return FALSE;
    }

    return TRUE;
}