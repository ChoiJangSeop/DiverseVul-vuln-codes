void *Type_ProfileSequenceDesc_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsUInt32Number* nItems, cmsUInt32Number SizeOfTag)
{
    cmsSEQ* OutSeq;
    cmsUInt32Number i, Count;

    *nItems = 0;

    if (!_cmsReadUInt32Number(io, &Count)) return NULL;

    if (SizeOfTag < sizeof(cmsUInt32Number)) return NULL;
    SizeOfTag -= sizeof(cmsUInt32Number);


    OutSeq = cmsAllocProfileSequenceDescription(self ->ContextID, Count);
    if (OutSeq == NULL) return NULL;

    OutSeq ->n = Count;

    // Get structures as well

    for (i=0; i < Count; i++) {

        cmsPSEQDESC* sec = &OutSeq -> seq[i];

        if (!_cmsReadUInt32Number(io, &sec ->deviceMfg)) goto Error;
        if (SizeOfTag < sizeof(cmsUInt32Number)) goto Error;
        SizeOfTag -= sizeof(cmsUInt32Number);

        if (!_cmsReadUInt32Number(io, &sec ->deviceModel)) goto Error;
        if (SizeOfTag < sizeof(cmsUInt32Number)) goto Error;
        SizeOfTag -= sizeof(cmsUInt32Number);

        if (!_cmsReadUInt64Number(io, &sec ->attributes)) goto Error;
        if (SizeOfTag < sizeof(cmsUInt32Number)) goto Error;
        SizeOfTag -= sizeof(cmsUInt64Number);

        if (!_cmsReadUInt32Number(io, (cmsUInt32Number *)&sec ->technology)) goto Error;
        if (SizeOfTag < sizeof(cmsUInt32Number)) goto Error;
        SizeOfTag -= sizeof(cmsUInt32Number);

        if (!ReadEmbeddedText(self, io, &sec ->Manufacturer, SizeOfTag)) goto Error;
        if (!ReadEmbeddedText(self, io, &sec ->Model, SizeOfTag)) goto Error;
    }

    *nItems = 1;
    return OutSeq;

Error:
    cmsFreeProfileSequenceDescription(OutSeq);
    return NULL;
}