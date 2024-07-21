cmsBool  Read16bitTables(cmsContext ContextID, cmsIOHANDLER* io, cmsPipeline* lut, int nChannels, int nEntries)
{
    cmsStage* mpe;
    int i;
    cmsToneCurve* Tables[cmsMAXCHANNELS];

    // Maybe an empty table? (this is a lcms extension)
    if (nEntries <= 0) return TRUE;

    // Check for malicious profiles
    if (nEntries < 2) return FALSE;
    if (nChannels > cmsMAXCHANNELS) return FALSE;

    // Init table to zero
    memset(Tables, 0, sizeof(Tables));

    for (i=0; i < nChannels; i++) {

        Tables[i] = cmsBuildTabulatedToneCurve16(ContextID, nEntries, NULL);
        if (Tables[i] == NULL) goto Error;

        if (!_cmsReadUInt16Array(io, nEntries, Tables[i]->Table16)) goto Error;
    }


    // Add the table (which may certainly be an identity, but this is up to the optimizer, not the reading code)
    mpe = cmsStageAllocToneCurves(ContextID, nChannels, Tables);
    if (mpe == NULL) goto Error;

    cmsPipelineInsertStage(lut, cmsAT_END, mpe);

    for (i=0; i < nChannels; i++)
        cmsFreeToneCurve(Tables[i]);

    return TRUE;

Error:
    for (i=0; i < nChannels; i++) {
        if (Tables[i]) cmsFreeToneCurve(Tables[i]);
    }

    return FALSE;
}