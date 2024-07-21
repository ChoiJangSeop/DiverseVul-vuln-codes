cmsPipeline* _cmsReadDevicelinkLUT(cmsHPROFILE hProfile, int Intent)
{
    cmsPipeline* Lut;
    cmsTagTypeSignature OriginalType;
    cmsTagSignature tag16    = Device2PCS16[Intent];
    cmsTagSignature tagFloat = Device2PCSFloat[Intent];
    cmsContext ContextID = cmsGetProfileContextID(hProfile);


    // On named color, take the appropiate tag
    if (cmsGetDeviceClass(hProfile) == cmsSigNamedColorClass) {

        cmsNAMEDCOLORLIST* nc = (cmsNAMEDCOLORLIST*) cmsReadTag(hProfile, cmsSigNamedColor2Tag);

        if (nc == NULL) return NULL;

        Lut = cmsPipelineAlloc(ContextID, 0, 0);
        if (Lut == NULL) {
            cmsFreeNamedColorList(nc);
            return NULL;
        }

        cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocNamedColor(nc, FALSE));
        if (cmsGetColorSpace(hProfile) == cmsSigLabData)
              cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV2ToV4(ContextID));
        return Lut;
    }

    if (cmsIsTag(hProfile, tagFloat)) {  // Float tag takes precedence

        // Floating point LUT are always V
        return _cmsReadFloatDevicelinkTag(hProfile, tagFloat);
    }

    tagFloat = Device2PCSFloat[0];
    if (cmsIsTag(hProfile, tagFloat)) {

        return cmsPipelineDup((cmsPipeline*) cmsReadTag(hProfile, tagFloat));
    }

    if (!cmsIsTag(hProfile, tag16)) {  // Is there any LUT-Based table?

        tag16    = Device2PCS16[0];
        if (!cmsIsTag(hProfile, tag16)) return NULL;
    }

    // Check profile version and LUT type. Do the necessary adjustments if needed

    // Read the tag
    Lut = (cmsPipeline*) cmsReadTag(hProfile, tag16);
    if (Lut == NULL) return NULL;

    // The profile owns the Lut, so we need to copy it
    Lut = cmsPipelineDup(Lut);
    if (Lut == NULL) return NULL;

     // Now it is time for a controversial stuff. I found that for 3D LUTS using
     // Lab used as indexer space,  trilinear interpolation should be used
    if (cmsGetColorSpace(hProfile) == cmsSigLabData)
                        ChangeInterpolationToTrilinear(Lut);

    // After reading it, we have info about the original type
    OriginalType =  _cmsGetTagTrueType(hProfile, tag16);

    // We need to adjust data for Lab16 on output
    if (OriginalType != cmsSigLut16Type) return Lut;

    // Here it is possible to get Lab on both sides

    if (cmsGetPCS(hProfile) == cmsSigLabData) {
            cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocLabV4ToV2(ContextID));
    }

    if (cmsGetColorSpace(hProfile) == cmsSigLabData) {
            cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV2ToV4(ContextID));
    }

    return Lut;


}