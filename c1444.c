cmsBool SaveTags(_cmsICCPROFILE* Icc, _cmsICCPROFILE* FileOrig)
{
    cmsUInt8Number* Data;
    cmsUInt32Number i;
    cmsUInt32Number Begin;
    cmsIOHANDLER* io = Icc ->IOhandler;
    cmsTagDescriptor* TagDescriptor;
    cmsTagTypeSignature TypeBase;
    cmsTagTypeHandler* TypeHandler;


    for (i=0; i < Icc -> TagCount; i++) {


        if (Icc ->TagNames[i] == 0) continue;

        // Linked tags are not written
        if (Icc ->TagLinked[i] != (cmsTagSignature) 0) continue;

        Icc -> TagOffsets[i] = Begin = io ->UsedSpace;

        Data = (cmsUInt8Number*)  Icc -> TagPtrs[i];

        if (!Data) {

            // Reach here if we are copying a tag from a disk-based ICC profile which has not been modified by user.
            // In this case a blind copy of the block data is performed
            if (FileOrig != NULL && Icc -> TagOffsets[i]) {

                cmsUInt32Number TagSize   = FileOrig -> TagSizes[i];
                cmsUInt32Number TagOffset = FileOrig -> TagOffsets[i];
                void* Mem;

                if (!FileOrig ->IOhandler->Seek(FileOrig ->IOhandler, TagOffset)) return FALSE;

                Mem = _cmsMalloc(Icc ->ContextID, TagSize);
                if (Mem == NULL) return FALSE;

                if (FileOrig ->IOhandler->Read(FileOrig->IOhandler, Mem, TagSize, 1) != 1) return FALSE;
                if (!io ->Write(io, TagSize, Mem)) return FALSE;
                _cmsFree(Icc ->ContextID, Mem);

                Icc -> TagSizes[i] = (io ->UsedSpace - Begin);


                // Align to 32 bit boundary.
                if (! _cmsWriteAlignment(io))
                    return FALSE;
            }

            continue;
        }


        // Should this tag be saved as RAW? If so, tagsizes should be specified in advance (no further cooking is done)
        if (Icc ->TagSaveAsRaw[i]) {

            if (io -> Write(io, Icc ->TagSizes[i], Data) != 1) return FALSE;
        }
        else {

            // Search for support on this tag
            TagDescriptor = _cmsGetTagDescriptor(Icc -> TagNames[i]);
            if (TagDescriptor == NULL) continue;                        // Unsupported, ignore it

            TypeHandler = Icc ->TagTypeHandlers[i];

            if (TypeHandler == NULL) {
                cmsSignalError(Icc ->ContextID, cmsERROR_INTERNAL, "(Internal) no handler for tag %x", Icc -> TagNames[i]);
                continue;
            }

            TypeBase = TypeHandler ->Signature;
            if (!_cmsWriteTypeBase(io, TypeBase))
                return FALSE;

            TypeHandler ->ContextID  = Icc ->ContextID;
            TypeHandler ->ICCVersion = Icc ->Version;
            if (!TypeHandler ->WritePtr(TypeHandler, io, Data, TagDescriptor ->ElemCount)) {

				char String[5];

				 _cmsTagSignature2String(String, (cmsTagSignature) TypeBase);
                cmsSignalError(Icc ->ContextID, cmsERROR_WRITE, "Couldn't write type '%s'", String);
                return FALSE;
            }
        }


        Icc -> TagSizes[i] = (io ->UsedSpace - Begin);

        // Align to 32 bit boundary.
        if (! _cmsWriteAlignment(io))
            return FALSE;
    }


    return TRUE;
}