cmsBool WritePositionTable(struct _cms_typehandler_struct* self,
                               cmsIOHANDLER* io,
                               cmsUInt32Number SizeOfTag,
                               cmsUInt32Number Count,
                               cmsUInt32Number BaseOffset,
                               void *Cargo,
                               PositionTableEntryFn ElementFn)
{
    cmsUInt32Number i;
    cmsUInt32Number DirectoryPos, CurrentPos, Before;
    cmsUInt32Number *ElementOffsets = NULL, *ElementSizes = NULL;

     // Create table
    ElementOffsets = (cmsUInt32Number *) _cmsCalloc(io ->ContextID, Count, sizeof(cmsUInt32Number *));
    if (ElementOffsets == NULL) goto Error;

    ElementSizes = (cmsUInt32Number *) _cmsCalloc(io ->ContextID, Count, sizeof(cmsUInt32Number *));
    if (ElementSizes == NULL) goto Error;

    // Keep starting position of curve offsets
    DirectoryPos = io ->Tell(io);

    // Write a fake directory to be filled latter on
    for (i=0; i < Count; i++) {

        if (!_cmsWriteUInt32Number(io, 0)) goto Error;  // Offset
        if (!_cmsWriteUInt32Number(io, 0)) goto Error;  // size
    }

    // Write each element. Keep track of the size as well.
    for (i=0; i < Count; i++) {

        Before = io ->Tell(io);
        ElementOffsets[i] = Before - BaseOffset;

        // Callback to write...
        if (!ElementFn(self, io, Cargo, i, SizeOfTag)) goto Error;

        // Now the size
        ElementSizes[i] = io ->Tell(io) - Before;
    }

    // Write the directory
    CurrentPos = io ->Tell(io);
    if (!io ->Seek(io, DirectoryPos)) goto Error;

    for (i=0; i <  Count; i++) {
        if (!_cmsWriteUInt32Number(io, ElementOffsets[i])) goto Error;
        if (!_cmsWriteUInt32Number(io, ElementSizes[i])) goto Error;
    }

    if (!io ->Seek(io, CurrentPos)) goto Error;

    if (ElementOffsets != NULL) _cmsFree(io ->ContextID, ElementOffsets);
    if (ElementSizes != NULL) _cmsFree(io ->ContextID, ElementSizes);
    return TRUE;

Error:
    if (ElementOffsets != NULL) _cmsFree(io ->ContextID, ElementOffsets);
    if (ElementSizes != NULL) _cmsFree(io ->ContextID, ElementSizes);
    return FALSE;
}