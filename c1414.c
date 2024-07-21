cmsHPROFILE CMSEXPORT cmsCreateXYZProfileTHR(cmsContext ContextID)
{
    cmsHPROFILE hProfile;
    cmsPipeline* LUT = NULL;

    hProfile = cmsCreateRGBProfileTHR(ContextID, cmsD50_xyY(), NULL, NULL);
    if (hProfile == NULL) return NULL;

    cmsSetProfileVersion(hProfile, 4.3);

    cmsSetDeviceClass(hProfile, cmsSigAbstractClass);
    cmsSetColorSpace(hProfile,  cmsSigXYZData);
    cmsSetPCS(hProfile,         cmsSigXYZData);

    if (!SetTextTags(hProfile, L"XYZ identity built-in")) goto Error;

    // An identity LUT is all we need
    LUT = cmsPipelineAlloc(ContextID, 3, 3);
    if (LUT == NULL) goto Error;

    cmsPipelineInsertStage(LUT, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, 3));

    if (!cmsWriteTag(hProfile, cmsSigAToB0Tag, LUT)) goto Error;
    cmsPipelineFree(LUT);

    return hProfile;

Error:

    if (LUT != NULL)
        cmsPipelineFree(LUT);

    if (hProfile != NULL)
        cmsCloseProfile(hProfile);

    return NULL;
}