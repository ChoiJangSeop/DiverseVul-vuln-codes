cmsPipeline* _cmsReadInputLUT(cmsHPROFILE hProfile, int Intent)
{
    cmsTagTypeSignature OriginalType;
    cmsTagSignature tag16    = Device2PCS16[Intent];
    cmsTagSignature tagFloat = Device2PCSFloat[Intent];
    cmsContext ContextID = cmsGetProfileContextID(hProfile);

    // On named color, take the appropiate tag
    if (cmsGetDeviceClass(hProfile) == cmsSigNamedColorClass) {

        cmsPipeline* Lut;
        cmsNAMEDCOLORLIST* nc = (cmsNAMEDCOLORLIST*) cmsReadTag(hProfile, cmsSigNamedColor2Tag);

        if (nc == NULL) return NULL;

        Lut = cmsPipelineAlloc(ContextID, 0, 0);
        if (Lut == NULL) {
            cmsFreeNamedColorList(nc);
            return NULL;
        }

        cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocNamedColor(nc, TRUE));
        cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV2ToV4(ContextID));
        return Lut;
    }

    if (cmsIsTag(hProfile, tagFloat)) {  // Float tag takes precedence

        // Floating point LUT are always V4, but the encoding range is no
        // longer 0..1.0, so we need to add an stage depending on the color space
         return _cmsReadFloatInputTag(hProfile, tagFloat);
    }

    // Revert to perceptual if no tag is found
    if (!cmsIsTag(hProfile, tag16)) {
        tag16 = Device2PCS16[0];
    }

    if (cmsIsTag(hProfile, tag16)) { // Is there any LUT-Based table?

        // Check profile version and LUT type. Do the necessary adjustments if needed

        // First read the tag
        cmsPipeline* Lut = (cmsPipeline*) cmsReadTag(hProfile, tag16);
        if (Lut == NULL) return NULL;

        // After reading it, we have now info about the original type
        OriginalType =  _cmsGetTagTrueType(hProfile, tag16);

        // The profile owns the Lut, so we need to copy it
        Lut = cmsPipelineDup(Lut);

        // We need to adjust data only for Lab16 on output
        if (OriginalType != cmsSigLut16Type || cmsGetPCS(hProfile) != cmsSigLabData)
            return Lut;

        // If the input is Lab, add also a conversion at the begin
        if (cmsGetColorSpace(hProfile) == cmsSigLabData)
            cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocLabV4ToV2(ContextID));

        // Add a matrix for conversion V2 to V4 Lab PCS
        cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV2ToV4(ContextID));
        return Lut;
    }

    // Lut was not found, try to create a matrix-shaper

    // Check if this is a grayscale profile.
    if (cmsGetColorSpace(hProfile) == cmsSigGrayData) {

        // if so, build appropiate conversion tables.
        // The tables are the PCS iluminant, scaled across GrayTRC
        return BuildGrayInputMatrixPipeline(hProfile);
    }

    // Not gray, create a normal matrix-shaper
    return BuildRGBInputMatrixShaper(hProfile);
}