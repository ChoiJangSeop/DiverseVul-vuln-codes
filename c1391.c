cmsHPROFILE CMSEXPORT cmsCreateLinearizationDeviceLinkTHR(cmsContext ContextID,
                                                          cmsColorSpaceSignature ColorSpace,
                                                          cmsToneCurve* const TransferFunctions[])
{
    cmsHPROFILE hICC;
    cmsPipeline* Pipeline;
    cmsStage* Lin;
    int nChannels;

    hICC = cmsCreateProfilePlaceholder(ContextID);
    if (!hICC)
        return NULL;

    cmsSetProfileVersion(hICC, 4.3);

    cmsSetDeviceClass(hICC,      cmsSigLinkClass);
    cmsSetColorSpace(hICC,       ColorSpace);
    cmsSetPCS(hICC,              ColorSpace);

    cmsSetHeaderRenderingIntent(hICC,  INTENT_PERCEPTUAL);

    // Set up channels
    nChannels = cmsChannelsOf(ColorSpace);

    // Creates a Pipeline with prelinearization step only
    Pipeline = cmsPipelineAlloc(ContextID, nChannels, nChannels);
    if (Pipeline == NULL) goto Error;


    // Copy tables to Pipeline
    Lin = cmsStageAllocToneCurves(ContextID, nChannels, TransferFunctions);
    if (Lin == NULL) goto Error;

    cmsPipelineInsertStage(Pipeline, cmsAT_BEGIN, Lin);

    // Create tags
    if (!SetTextTags(hICC, L"Linearization built-in")) goto Error;
    if (!cmsWriteTag(hICC, cmsSigAToB0Tag, (void*) Pipeline)) goto Error;
    if (!SetSeqDescTag(hICC, "Linearization built-in")) goto Error;

    // Pipeline is already on virtual profile
    cmsPipelineFree(Pipeline);

    // Ok, done
    return hICC;

Error:
    if (hICC)
        cmsCloseProfile(hICC);


    return NULL;
}