cmsPipeline* BlackPreservingKPlaneIntents(cmsContext     ContextID,
                                          cmsUInt32Number nProfiles,
                                          cmsUInt32Number TheIntents[],
                                          cmsHPROFILE     hProfiles[],
                                          cmsBool         BPC[],
                                          cmsFloat64Number AdaptationStates[],
                                          cmsUInt32Number dwFlags)
{
    PreserveKPlaneParams bp;
    cmsPipeline*    Result = NULL;
    cmsUInt32Number ICCIntents[256];
    cmsStage*         CLUT;
    cmsUInt32Number i, nGridPoints;
    cmsHPROFILE hLab;

    // Sanity check
    if (nProfiles < 1 || nProfiles > 255) return NULL;

    // Translate black-preserving intents to ICC ones
    for (i=0; i < nProfiles; i++)
        ICCIntents[i] = TranslateNonICCIntents(TheIntents[i]);

    // Check for non-cmyk profiles
    if (cmsGetColorSpace(hProfiles[0]) != cmsSigCmykData ||
        !(cmsGetColorSpace(hProfiles[nProfiles-1]) == cmsSigCmykData ||
        cmsGetDeviceClass(hProfiles[nProfiles-1]) == cmsSigOutputClass))
           return  DefaultICCintents(ContextID, nProfiles, ICCIntents, hProfiles, BPC, AdaptationStates, dwFlags);

    // Allocate an empty LUT for holding the result
    Result = cmsPipelineAlloc(ContextID, 4, 4);
    if (Result == NULL) return NULL;


    memset(&bp, 0, sizeof(bp));

    // We need the input LUT of the last profile, assuming this one is responsible of
    // black generation. This LUT will be seached in inverse order.
    bp.LabK2cmyk = _cmsReadInputLUT(hProfiles[nProfiles-1], INTENT_RELATIVE_COLORIMETRIC);
    if (bp.LabK2cmyk == NULL) goto Cleanup;

    // Get total area coverage (in 0..1 domain)
    bp.MaxTAC = cmsDetectTAC(hProfiles[nProfiles-1]) / 100.0;
    if (bp.MaxTAC <= 0) goto Cleanup;


    // Create a LUT holding normal ICC transform
    bp.cmyk2cmyk = DefaultICCintents(ContextID,
                                         nProfiles,
                                         ICCIntents,
                                         hProfiles,
                                         BPC,
                                         AdaptationStates,
                                         dwFlags);
    if (bp.cmyk2cmyk == NULL) goto Cleanup;

    // Now the tone curve
    bp.KTone = _cmsBuildKToneCurve(ContextID, 4096, nProfiles,
                                   ICCIntents,
                                   hProfiles,
                                   BPC,
                                   AdaptationStates,
                                   dwFlags);
    if (bp.KTone == NULL) goto Cleanup;

    // To measure the output, Last profile to Lab
    hLab = cmsCreateLab4ProfileTHR(ContextID, NULL);
    bp.hProofOutput = cmsCreateTransformTHR(ContextID, hProfiles[nProfiles-1],
                                         CHANNELS_SH(4)|BYTES_SH(2), hLab, TYPE_Lab_DBL,
                                         INTENT_RELATIVE_COLORIMETRIC,
                                         cmsFLAGS_NOCACHE|cmsFLAGS_NOOPTIMIZE);
    if ( bp.hProofOutput == NULL) goto Cleanup;

    // Same as anterior, but lab in the 0..1 range
    bp.cmyk2Lab = cmsCreateTransformTHR(ContextID, hProfiles[nProfiles-1],
                                         FLOAT_SH(1)|CHANNELS_SH(4)|BYTES_SH(4), hLab,
                                         FLOAT_SH(1)|CHANNELS_SH(3)|BYTES_SH(4),
                                         INTENT_RELATIVE_COLORIMETRIC,
                                         cmsFLAGS_NOCACHE|cmsFLAGS_NOOPTIMIZE);
    if (bp.cmyk2Lab == NULL) goto Cleanup;
    cmsCloseProfile(hLab);

    // Error estimation (for debug only)
    bp.MaxError = 0;

    // How many gridpoints are we going to use?
    nGridPoints = _cmsReasonableGridpointsByColorspace(cmsSigCmykData, dwFlags);


    CLUT = cmsStageAllocCLut16bit(ContextID, nGridPoints, 4, 4, NULL);
    if (CLUT == NULL) goto Cleanup;

    cmsPipelineInsertStage(Result, cmsAT_BEGIN, CLUT);

    cmsStageSampleCLut16bit(CLUT, BlackPreservingSampler, (void*) &bp, 0);

Cleanup:

    if (bp.cmyk2cmyk) cmsPipelineFree(bp.cmyk2cmyk);
    if (bp.cmyk2Lab) cmsDeleteTransform(bp.cmyk2Lab);
    if (bp.hProofOutput) cmsDeleteTransform(bp.hProofOutput);

    if (bp.KTone) cmsFreeToneCurve(bp.KTone);
    if (bp.LabK2cmyk) cmsPipelineFree(bp.LabK2cmyk);

    return Result;
}