cmsHPROFILE CMSEXPORT cmsCreateNULLProfileTHR(cmsContext ContextID)
{
    cmsHPROFILE hProfile;
    cmsPipeline* LUT = NULL;
    cmsStage* PostLin;
    cmsToneCurve* EmptyTab;
    cmsUInt16Number Zero[2] = { 0, 0 };

    hProfile = cmsCreateProfilePlaceholder(ContextID);
    if (!hProfile)                          // can't allocate
        return NULL;

    cmsSetProfileVersion(hProfile, 4.3);

    if (!SetTextTags(hProfile, L"NULL profile built-in")) goto Error;



    cmsSetDeviceClass(hProfile, cmsSigOutputClass);
    cmsSetColorSpace(hProfile,  cmsSigGrayData);
    cmsSetPCS(hProfile,         cmsSigLabData);

    // An empty LUTs is all we need
    LUT = cmsPipelineAlloc(ContextID, 1, 1);
    if (LUT == NULL) goto Error;

    EmptyTab = cmsBuildTabulatedToneCurve16(ContextID, 2, Zero);
    PostLin = cmsStageAllocToneCurves(ContextID, 1, &EmptyTab);
    cmsFreeToneCurve(EmptyTab);

    cmsPipelineInsertStage(LUT, cmsAT_END, PostLin);

    if (!cmsWriteTag(hProfile, cmsSigBToA0Tag, (void*) LUT)) goto Error;
    if (!cmsWriteTag(hProfile, cmsSigMediaWhitePointTag, cmsD50_XYZ())) goto Error;

    cmsPipelineFree(LUT);
    return hProfile;

Error:

    if (LUT != NULL)
        cmsPipelineFree(LUT);

    if (hProfile != NULL)
        cmsCloseProfile(hProfile);

    return NULL;
}