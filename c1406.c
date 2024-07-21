cmsPipeline* _cmsReadFloatOutputTag(cmsHPROFILE hProfile, cmsTagSignature tagFloat)
{
    cmsContext ContextID       = cmsGetProfileContextID(hProfile);
    cmsPipeline* Lut           = cmsPipelineDup((cmsPipeline*) cmsReadTag(hProfile, tagFloat));
    cmsColorSpaceSignature PCS = cmsGetPCS(hProfile);
    cmsColorSpaceSignature dataSpace = cmsGetColorSpace(hProfile);
    
    if (Lut == NULL) return NULL;
    
    // If PCS is Lab or XYZ, the floating point tag is accepting data in the space encoding,
    // and since the formatter has already accomodated to 0..1.0, we should undo this change
    if ( PCS == cmsSigLabData)
    {
        cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageNormalizeToLabFloat(ContextID));
    }
    else
        if (PCS == cmsSigXYZData)
        {
            cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageNormalizeToXyzFloat(ContextID));
        }
    
    // the output can be Lab or XYZ, in which case normalisation is needed on the end of the pipeline
    if ( dataSpace == cmsSigLabData)
    {
        cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageNormalizeFromLabFloat(ContextID));
    }
    else if ( dataSpace == cmsSigXYZData)
    {
        cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageNormalizeFromXyzFloat(ContextID));
    }
    
    return Lut;
}