cmsHPROFILE CMSEXPORT cmsCreateInkLimitingDeviceLinkTHR(cmsContext ContextID,
                                                     cmsColorSpaceSignature ColorSpace,
                                                     cmsFloat64Number Limit)
{
    cmsHPROFILE hICC;
    cmsPipeline* LUT;
    cmsStage* CLUT;
    int nChannels;

    if (ColorSpace != cmsSigCmykData) {
        cmsSignalError(ContextID, cmsERROR_COLORSPACE_CHECK, "InkLimiting: Only CMYK currently supported");
        return NULL;
    }

    if (Limit < 0.0 || Limit > 400) {

        cmsSignalError(ContextID, cmsERROR_RANGE, "InkLimiting: Limit should be between 0..400");
        if (Limit < 0) Limit = 0;
        if (Limit > 400) Limit = 400;

    }

    hICC = cmsCreateProfilePlaceholder(ContextID);
    if (!hICC)                          // can't allocate
        return NULL;

    cmsSetProfileVersion(hICC, 4.3);

    cmsSetDeviceClass(hICC,      cmsSigLinkClass);
    cmsSetColorSpace(hICC,       ColorSpace);
    cmsSetPCS(hICC,              ColorSpace);

    cmsSetHeaderRenderingIntent(hICC,  INTENT_PERCEPTUAL);


    // Creates a Pipeline with 3D grid only
    LUT = cmsPipelineAlloc(ContextID, 4, 4);
    if (LUT == NULL) goto Error;


    nChannels = cmsChannelsOf(ColorSpace);

    CLUT = cmsStageAllocCLut16bit(ContextID, 17, nChannels, nChannels, NULL);
    if (CLUT == NULL) goto Error;

    if (!cmsStageSampleCLut16bit(CLUT, InkLimitingSampler, (void*) &Limit, 0)) goto Error;

    cmsPipelineInsertStage(LUT, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, nChannels));
    cmsPipelineInsertStage(LUT, cmsAT_END, CLUT);
    cmsPipelineInsertStage(LUT, cmsAT_END, _cmsStageAllocIdentityCurves(ContextID, nChannels));

    // Create tags
    if (!SetTextTags(hICC, L"ink-limiting built-in")) goto Error;

    if (!cmsWriteTag(hICC, cmsSigAToB0Tag, (void*) LUT))  goto Error;
    if (!SetSeqDescTag(hICC, "ink-limiting built-in")) goto Error;

    // cmsPipeline is already on virtual profile
    cmsPipelineFree(LUT);

    // Ok, done
    return hICC;

Error:
    if (LUT != NULL)
        cmsPipelineFree(LUT);

    if (hICC != NULL)
        cmsCloseProfile(hICC);

    return NULL;
}