cmsPipeline* _cmsReadOutputLUT(cmsHPROFILE hProfile, int Intent)
{
    cmsTagTypeSignature OriginalType;
    cmsTagSignature tag16    = PCS2Device16[Intent];
    cmsTagSignature tagFloat = PCS2DeviceFloat[Intent];
    cmsContext ContextID     = cmsGetProfileContextID(hProfile);

    if (cmsIsTag(hProfile, tagFloat)) {  // Float tag takes precedence

        // Floating point LUT are always V4
        return _cmsReadFloatOutputTag(hProfile, tagFloat);
    }

    // Revert to perceptual if no tag is found
    if (!cmsIsTag(hProfile, tag16)) {
        tag16 = PCS2Device16[0];
    }

    if (cmsIsTag(hProfile, tag16)) { // Is there any LUT-Based table?

        // Check profile version and LUT type. Do the necessary adjustments if needed

        // First read the tag
        cmsPipeline* Lut = (cmsPipeline*) cmsReadTag(hProfile, tag16);
        if (Lut == NULL) return NULL;

        // After reading it, we have info about the original type
        OriginalType =  _cmsGetTagTrueType(hProfile, tag16);

        // The profile owns the Lut, so we need to copy it
        Lut = cmsPipelineDup(Lut);
        if (Lut == NULL) return NULL;

        // Now it is time for a controversial stuff. I found that for 3D LUTS using
        // Lab used as indexer space,  trilinear interpolation should be used
        if (cmsGetPCS(hProfile) == cmsSigLabData)
                             ChangeInterpolationToTrilinear(Lut);

        // We need to adjust data only for Lab and Lut16 type
        if (OriginalType != cmsSigLut16Type || cmsGetPCS(hProfile) != cmsSigLabData)
            return Lut;

        // Add a matrix for conversion V4 to V2 Lab PCS
        cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocLabV4ToV2(ContextID));

        // If the output is Lab, add also a conversion at the end
        if (cmsGetColorSpace(hProfile) == cmsSigLabData)
            cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV2ToV4(ContextID));

        return Lut;
    }

    // Lut not found, try to create a matrix-shaper

    // Check if this is a grayscale profile.
     if (cmsGetColorSpace(hProfile) == cmsSigGrayData) {

              // if so, build appropiate conversion tables.
              // The tables are the PCS iluminant, scaled across GrayTRC
              return BuildGrayOutputPipeline(hProfile);
    }

    // Not gray, create a normal matrix-shaper, which only operates in XYZ space  
    return BuildRGBOutputMatrixShaper(hProfile);
}