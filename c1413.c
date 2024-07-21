cmsPipeline*  BlackPreservingKOnlyIntents(cmsContext     ContextID,
                                          cmsUInt32Number nProfiles,
                                          cmsUInt32Number TheIntents[],
                                          cmsHPROFILE     hProfiles[],
                                          cmsBool         BPC[],
                                          cmsFloat64Number AdaptationStates[],
                                          cmsUInt32Number dwFlags)
{
    GrayOnlyParams  bp;
    cmsPipeline*    Result;
    cmsUInt32Number ICCIntents[256];
    cmsStage*         CLUT;
    cmsUInt32Number i, nGridPoints;


    // Sanity check
    if (nProfiles < 1 || nProfiles > 255) return NULL;

    // Translate black-preserving intents to ICC ones
    for (i=0; i < nProfiles; i++)
        ICCIntents[i] = TranslateNonICCIntents(TheIntents[i]);

    // Check for non-cmyk profiles
    if (cmsGetColorSpace(hProfiles[0]) != cmsSigCmykData ||
        cmsGetColorSpace(hProfiles[nProfiles-1]) != cmsSigCmykData)
           return DefaultICCintents(ContextID, nProfiles, ICCIntents, hProfiles, BPC, AdaptationStates, dwFlags);

    memset(&bp, 0, sizeof(bp));

    // Allocate an empty LUT for holding the result
    Result = cmsPipelineAlloc(ContextID, 4, 4);
    if (Result == NULL) return NULL;

    // Create a LUT holding normal ICC transform
    bp.cmyk2cmyk = DefaultICCintents(ContextID,
        nProfiles,
        ICCIntents,
        hProfiles,
        BPC,
        AdaptationStates,
        dwFlags);

    if (bp.cmyk2cmyk == NULL) goto Error;

    // Now, compute the tone curve
    bp.KTone = _cmsBuildKToneCurve(ContextID,
        4096,
        nProfiles,
        ICCIntents,
        hProfiles,
        BPC,
        AdaptationStates,
        dwFlags);

    if (bp.KTone == NULL) goto Error;


    // How many gridpoints are we going to use?
    nGridPoints = _cmsReasonableGridpointsByColorspace(cmsSigCmykData, dwFlags);

    // Create the CLUT. 16 bits
    CLUT = cmsStageAllocCLut16bit(ContextID, nGridPoints, 4, 4, NULL);
    if (CLUT == NULL) goto Error;

    // This is the one and only MPE in this LUT
    cmsPipelineInsertStage(Result, cmsAT_BEGIN, CLUT);

    // Sample it. We cannot afford pre/post linearization this time.
    if (!cmsStageSampleCLut16bit(CLUT, BlackPreservingGrayOnlySampler, (void*) &bp, 0))
        goto Error;

    // Get rid of xform and tone curve
    cmsPipelineFree(bp.cmyk2cmyk);
    cmsFreeToneCurve(bp.KTone);

    return Result;

Error:

    if (bp.cmyk2cmyk != NULL) cmsPipelineFree(bp.cmyk2cmyk);
    if (bp.KTone != NULL)  cmsFreeToneCurve(bp.KTone);
    if (Result != NULL) cmsPipelineFree(Result);
    return NULL;

}