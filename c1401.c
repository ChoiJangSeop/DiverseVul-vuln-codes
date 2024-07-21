cmsHPROFILE CMSEXPORT cmsCreateLab2ProfileTHR(cmsContext ContextID, const cmsCIExyY* WhitePoint)
{
    cmsHPROFILE hProfile;
    cmsPipeline* LUT = NULL;

    hProfile = cmsCreateRGBProfileTHR(ContextID, WhitePoint == NULL ? cmsD50_xyY() : WhitePoint, NULL, NULL);
    if (hProfile == NULL) return NULL;

    cmsSetProfileVersion(hProfile, 2.1);

    cmsSetDeviceClass(hProfile, cmsSigAbstractClass);
    cmsSetColorSpace(hProfile,  cmsSigLabData);
    cmsSetPCS(hProfile,         cmsSigLabData);

    if (!SetTextTags(hProfile, L"Lab identity built-in")) return NULL;

    // An identity LUT is all we need
    LUT = cmsPipelineAlloc(ContextID, 3, 3);
    if (LUT == NULL) goto Error;

    cmsPipelineInsertStage(LUT, cmsAT_BEGIN, _cmsStageAllocIdentityCLut(ContextID, 3));

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