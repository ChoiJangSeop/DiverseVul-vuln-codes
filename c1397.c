cmsHPROFILE CMSEXPORT cmsTransform2DeviceLink(cmsHTRANSFORM hTransform, cmsFloat64Number Version, cmsUInt32Number dwFlags)
{
    cmsHPROFILE hProfile = NULL;
    cmsUInt32Number FrmIn, FrmOut, ChansIn, ChansOut;
    cmsUInt32Number ColorSpaceBitsIn, ColorSpaceBitsOut;
    _cmsTRANSFORM* xform = (_cmsTRANSFORM*) hTransform;
    cmsPipeline* LUT = NULL;
    cmsStage* mpe;
    cmsContext ContextID = cmsGetTransformContextID(hTransform);
    const cmsAllowedLUT* AllowedLUT;
    cmsTagSignature DestinationTag;

    _cmsAssert(hTransform != NULL);

    // Get the first mpe to check for named color
    mpe = cmsPipelineGetPtrToFirstStage(xform ->Lut);

    // Check if is a named color transform
    if (mpe != NULL) {

        if (cmsStageType(mpe) == cmsSigNamedColorElemType) {
            return CreateNamedColorDevicelink(hTransform);
        }
    }

    // First thing to do is to get a copy of the transformation
    LUT = cmsPipelineDup(xform ->Lut);
    if (LUT == NULL) return NULL;

    // Time to fix the Lab2/Lab4 issue.
    if ((xform ->EntryColorSpace == cmsSigLabData) && (Version < 4.0)) {

        cmsPipelineInsertStage(LUT, cmsAT_BEGIN, _cmsStageAllocLabV2ToV4curves(ContextID));
    }

    // On the output side too
    if ((xform ->ExitColorSpace) == cmsSigLabData && (Version < 4.0)) {

        cmsPipelineInsertStage(LUT, cmsAT_END, _cmsStageAllocLabV4ToV2(ContextID));
    }


    hProfile = cmsCreateProfilePlaceholder(ContextID);
    if (!hProfile) goto Error;                    // can't allocate

    cmsSetProfileVersion(hProfile, Version);

    FixColorSpaces(hProfile, xform -> EntryColorSpace, xform -> ExitColorSpace, dwFlags);

    // Optimize the LUT and precalculate a devicelink

    ChansIn  = cmsChannelsOf(xform -> EntryColorSpace);
    ChansOut = cmsChannelsOf(xform -> ExitColorSpace);

    ColorSpaceBitsIn  = _cmsLCMScolorSpace(xform -> EntryColorSpace);
    ColorSpaceBitsOut = _cmsLCMScolorSpace(xform -> ExitColorSpace);

    FrmIn  = COLORSPACE_SH(ColorSpaceBitsIn) | CHANNELS_SH(ChansIn)|BYTES_SH(2);
    FrmOut = COLORSPACE_SH(ColorSpaceBitsOut) | CHANNELS_SH(ChansOut)|BYTES_SH(2);


     if (cmsGetDeviceClass(hProfile) == cmsSigOutputClass)
         DestinationTag = cmsSigBToA0Tag;
     else
         DestinationTag = cmsSigAToB0Tag;

    // Check if the profile/version can store the result
    if (dwFlags & cmsFLAGS_FORCE_CLUT)
        AllowedLUT = NULL;
    else
        AllowedLUT = FindCombination(LUT, Version >= 4.0, DestinationTag);

    if (AllowedLUT == NULL) {

        // Try to optimize
        _cmsOptimizePipeline(&LUT, xform ->RenderingIntent, &FrmIn, &FrmOut, &dwFlags);
        AllowedLUT = FindCombination(LUT, Version >= 4.0, DestinationTag);

    }

    // If no way, then force CLUT that for sure can be written
    if (AllowedLUT == NULL) {

        dwFlags |= cmsFLAGS_FORCE_CLUT;
        _cmsOptimizePipeline(&LUT, xform ->RenderingIntent, &FrmIn, &FrmOut, &dwFlags);

        // Put identity curves if needed
        if (cmsPipelineGetPtrToFirstStage(LUT) ->Type != cmsSigCurveSetElemType)
             cmsPipelineInsertStage(LUT, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, ChansIn));

        if (cmsPipelineGetPtrToLastStage(LUT) ->Type != cmsSigCurveSetElemType)
             cmsPipelineInsertStage(LUT, cmsAT_END,   _cmsStageAllocIdentityCurves(ContextID, ChansOut));

        AllowedLUT = FindCombination(LUT, Version >= 4.0, DestinationTag);
    }

    // Somethings is wrong...
    if (AllowedLUT == NULL) {
        goto Error;
    }


    if (dwFlags & cmsFLAGS_8BITS_DEVICELINK)
                     cmsPipelineSetSaveAs8bitsFlag(LUT, TRUE);

    // Tag profile with information
    if (!SetTextTags(hProfile, L"devicelink")) goto Error;

    // Store result
    if (!cmsWriteTag(hProfile, DestinationTag, LUT)) goto Error;


    if (xform -> InputColorant != NULL) {
           if (!cmsWriteTag(hProfile, cmsSigColorantTableTag, xform->InputColorant)) goto Error;
    }

    if (xform -> OutputColorant != NULL) {
           if (!cmsWriteTag(hProfile, cmsSigColorantTableOutTag, xform->OutputColorant)) goto Error;
    }

    if (xform ->Sequence != NULL) {
        if (!_cmsWriteProfileSequence(hProfile, xform ->Sequence)) goto Error;
    }

    cmsPipelineFree(LUT);
    return hProfile;

Error:
    if (LUT != NULL) cmsPipelineFree(LUT);
    cmsCloseProfile(hProfile);
    return NULL;
}