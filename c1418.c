cmsPipeline* BuildGrayOutputPipeline(cmsHPROFILE hProfile)
{
    cmsToneCurve *GrayTRC, *RevGrayTRC;
    cmsPipeline* Lut;
    cmsContext ContextID = cmsGetProfileContextID(hProfile);

    GrayTRC = (cmsToneCurve *) cmsReadTag(hProfile, cmsSigGrayTRCTag);
    if (GrayTRC == NULL) return NULL;

    RevGrayTRC = cmsReverseToneCurve(GrayTRC);
    if (RevGrayTRC == NULL) return NULL;

    Lut = cmsPipelineAlloc(ContextID, 3, 1);
    if (Lut == NULL) {
        cmsFreeToneCurve(RevGrayTRC);
        return NULL;
    }

    if (cmsGetPCS(hProfile) == cmsSigLabData) {

        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocMatrix(ContextID, 1,  3, PickLstarMatrix, NULL));
    }
    else  {
        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocMatrix(ContextID, 1,  3, PickYMatrix, NULL));
    }

    cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, 1, &RevGrayTRC));
    cmsFreeToneCurve(RevGrayTRC);

    return Lut;
}