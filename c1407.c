cmsBool ReadMPEElem(struct _cms_typehandler_struct* self,
                    cmsIOHANDLER* io,
                    void* Cargo,
                    cmsUInt32Number n,
                    cmsUInt32Number SizeOfTag)
{
    cmsStageSignature ElementSig;
    cmsTagTypeHandler* TypeHandler;
    cmsStage *mpe = NULL;
    cmsUInt32Number nItems;
    cmsPipeline *NewLUT = (cmsPipeline *) Cargo;

    // Take signature and channels for each element.
    if (!_cmsReadUInt32Number(io, (cmsUInt32Number*) &ElementSig)) return FALSE;

    // The reserved placeholder
    if (!_cmsReadUInt32Number(io, NULL)) return FALSE;

    // Read diverse MPE types
    TypeHandler = GetHandler((cmsTagTypeSignature) ElementSig, SupportedMPEtypes);
    if (TypeHandler == NULL)  {

        char String[5];

        _cmsTagSignature2String(String, (cmsTagSignature) ElementSig);

        // An unknown element was found.
        cmsSignalError(self ->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown MPE type '%s' found.", String);
        return FALSE;
    }

    // If no read method, just ignore the element (valid for cmsSigBAcsElemType and cmsSigEAcsElemType)
    // Read the MPE. No size is given
    if (TypeHandler ->ReadPtr != NULL) {

        // This is a real element which should be read and processed
        mpe = (cmsStage*) TypeHandler ->ReadPtr(self, io, &nItems, SizeOfTag);
        if (mpe == NULL) return FALSE;

        // All seems ok, insert element
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    return TRUE;

    cmsUNUSED_PARAMETER(SizeOfTag);
    cmsUNUSED_PARAMETER(n);
}