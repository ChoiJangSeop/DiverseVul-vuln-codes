cmsPipeline* BuildGrayInputMatrixPipeline(cmsHPROFILE hProfile)
{
    cmsToneCurve *GrayTRC;
    cmsPipeline* Lut;
    cmsContext ContextID = cmsGetProfileContextID(hProfile);

    GrayTRC = (cmsToneCurve *) cmsReadTag(hProfile, cmsSigGrayTRCTag);
    if (GrayTRC == NULL) return NULL;

    Lut = cmsPipelineAlloc(ContextID, 1, 3);
    if (Lut == NULL) return NULL;

    if (cmsGetPCS(hProfile) == cmsSigLabData) {

        // In this case we implement the profile as an  identity matrix plus 3 tone curves
        cmsUInt16Number Zero[2] = { 0x8080, 0x8080 };
        cmsToneCurve* EmptyTab;
        cmsToneCurve* LabCurves[3];

        EmptyTab = cmsBuildTabulatedToneCurve16(ContextID, 2, Zero);

        if (EmptyTab == NULL) {

                 cmsPipelineFree(Lut);
                 return NULL;
        }

        LabCurves[0] = GrayTRC;
        LabCurves[1] = EmptyTab;
        LabCurves[2] = EmptyTab;

        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocMatrix(ContextID, 3,  1, OneToThreeInputMatrix, NULL));
        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, 3, LabCurves));

        cmsFreeToneCurve(EmptyTab);

    }
    else  {
       cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, 1, &GrayTRC));
       cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocMatrix(ContextID, 3,  1, GrayInputMatrix, NULL));
    }

    return Lut;
}