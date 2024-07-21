cmsStage* CMSEXPORT cmsStageAllocCLut16bit(cmsContext ContextID,
                                    cmsUInt32Number nGridPoints,
                                    cmsUInt32Number inputChan,
                                    cmsUInt32Number outputChan,
                                    const cmsUInt16Number* Table)
{
    cmsUInt32Number Dimensions[MAX_INPUT_DIMENSIONS];
    int i;

   // Our resulting LUT would be same gridpoints on all dimensions
    for (i=0; i < MAX_INPUT_DIMENSIONS; i++)
        Dimensions[i] = nGridPoints;


    return cmsStageAllocCLut16bitGranular(ContextID, Dimensions, inputChan, outputChan, Table);
}