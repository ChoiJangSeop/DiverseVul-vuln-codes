cmsBool Type_MPE_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void* Ptr, cmsUInt32Number nItems)
{
    cmsUInt32Number i, BaseOffset, DirectoryPos, CurrentPos;
    int inputChan, outputChan;
    cmsUInt32Number ElemCount;
    cmsUInt32Number *ElementOffsets = NULL, *ElementSizes = NULL, Before;
    cmsStageSignature ElementSig;
    cmsPipeline* Lut = (cmsPipeline*) Ptr;
    cmsStage* Elem = Lut ->Elements;
    cmsTagTypeHandler* TypeHandler;

    BaseOffset = io ->Tell(io) - sizeof(_cmsTagBase);

    inputChan  = cmsPipelineInputChannels(Lut);
    outputChan = cmsPipelineOutputChannels(Lut);
    ElemCount  = cmsPipelineStageCount(Lut);

    ElementOffsets = (cmsUInt32Number *) _cmsCalloc(self ->ContextID, ElemCount, sizeof(cmsUInt32Number *));
    if (ElementOffsets == NULL) goto Error;

    ElementSizes = (cmsUInt32Number *) _cmsCalloc(self ->ContextID, ElemCount, sizeof(cmsUInt32Number *));
    if (ElementSizes == NULL) goto Error;

    // Write the head
    if (!_cmsWriteUInt16Number(io, (cmsUInt16Number) inputChan)) goto Error;
    if (!_cmsWriteUInt16Number(io, (cmsUInt16Number) outputChan)) goto Error;
    if (!_cmsWriteUInt32Number(io, (cmsUInt16Number) ElemCount)) goto Error;

    DirectoryPos = io ->Tell(io);

    // Write a fake directory to be filled latter on
    for (i=0; i < ElemCount; i++) {
        if (!_cmsWriteUInt32Number(io, 0)) goto Error;  // Offset
        if (!_cmsWriteUInt32Number(io, 0)) goto Error;  // size
    }

    // Write each single tag. Keep track of the size as well.
    for (i=0; i < ElemCount; i++) {

        ElementOffsets[i] = io ->Tell(io) - BaseOffset;

        ElementSig = Elem ->Type;

        TypeHandler = GetHandler((cmsTagTypeSignature) ElementSig, SupportedMPEtypes);
        if (TypeHandler == NULL)  {

                char String[5];

                _cmsTagSignature2String(String, (cmsTagSignature) ElementSig);

                 // An unknow element was found.
                 cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Found unknown MPE type '%s'", String);
                 goto Error;
        }

        if (!_cmsWriteUInt32Number(io, ElementSig)) goto Error;
        if (!_cmsWriteUInt32Number(io, 0)) goto Error;
        Before = io ->Tell(io);
        if (!TypeHandler ->WritePtr(self, io, Elem, 1)) goto Error;
        if (!_cmsWriteAlignment(io)) goto Error;

        ElementSizes[i] = io ->Tell(io) - Before;

        Elem = Elem ->Next;
    }

    // Write the directory
    CurrentPos = io ->Tell(io);

    if (!io ->Seek(io, DirectoryPos)) goto Error;

    for (i=0; i < ElemCount; i++) {
        if (!_cmsWriteUInt32Number(io, ElementOffsets[i])) goto Error;
        if (!_cmsWriteUInt32Number(io, ElementSizes[i])) goto Error;
    }

    if (!io ->Seek(io, CurrentPos)) goto Error;

    if (ElementOffsets != NULL) _cmsFree(self ->ContextID, ElementOffsets);
    if (ElementSizes != NULL) _cmsFree(self ->ContextID, ElementSizes);
    return TRUE;

Error:
    if (ElementOffsets != NULL) _cmsFree(self ->ContextID, ElementOffsets);
    if (ElementSizes != NULL) _cmsFree(self ->ContextID, ElementSizes);
    return FALSE;

    cmsUNUSED_PARAMETER(nItems);
}