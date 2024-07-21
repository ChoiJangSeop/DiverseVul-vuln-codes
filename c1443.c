void *Type_MPEclut_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsUInt32Number* nItems, cmsUInt32Number SizeOfTag)
{
    cmsStage* mpe = NULL;
    cmsUInt16Number InputChans, OutputChans;
    cmsUInt8Number Dimensions8[16];
    cmsUInt32Number i, nMaxGrids, GridPoints[MAX_INPUT_DIMENSIONS];
    _cmsStageCLutData* clut;

    if (!_cmsReadUInt16Number(io, &InputChans)) return NULL;
    if (!_cmsReadUInt16Number(io, &OutputChans)) return NULL;

    if (io ->Read(io, Dimensions8, sizeof(cmsUInt8Number), 16) != 16)
        goto Error;

    // Copy MAX_INPUT_DIMENSIONS at most. Expand to cmsUInt32Number
    nMaxGrids = InputChans > MAX_INPUT_DIMENSIONS ? MAX_INPUT_DIMENSIONS : InputChans;
    for (i=0; i < nMaxGrids; i++) GridPoints[i] = (cmsUInt32Number) Dimensions8[i];

    // Allocate the true CLUT
    mpe = cmsStageAllocCLutFloatGranular(self ->ContextID, GridPoints, InputChans, OutputChans, NULL);
    if (mpe == NULL) goto Error;

    // Read the data
    clut = (_cmsStageCLutData*) mpe ->Data;
    for (i=0; i < clut ->nEntries; i++) {

        if (!_cmsReadFloat32Number(io, &clut ->Tab.TFloat[i])) goto Error;
    }

    *nItems = 1;
    return mpe;

Error:
    *nItems = 0;
    if (mpe != NULL) cmsStageFree(mpe);
    return NULL;

    cmsUNUSED_PARAMETER(SizeOfTag);
}