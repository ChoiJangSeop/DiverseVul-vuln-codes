cmsPipeline* BuildRGBInputMatrixShaper(cmsHPROFILE hProfile)
{
    cmsPipeline* Lut;
    cmsMAT3 Mat;
    cmsToneCurve *Shapes[3];
    cmsContext ContextID = cmsGetProfileContextID(hProfile);
    int i, j;

    if (!ReadICCMatrixRGB2XYZ(&Mat, hProfile)) return NULL;

    // XYZ PCS in encoded in 1.15 format, and the matrix output comes in 0..0xffff range, so
    // we need to adjust the output by a factor of (0x10000/0xffff) to put data in
    // a 1.16 range, and then a >> 1 to obtain 1.15. The total factor is (65536.0)/(65535.0*2)

    for (i=0; i < 3; i++)
        for (j=0; j < 3; j++)
            Mat.v[i].n[j] *= InpAdj;


    Shapes[0] = (cmsToneCurve *) cmsReadTag(hProfile, cmsSigRedTRCTag);
    Shapes[1] = (cmsToneCurve *) cmsReadTag(hProfile, cmsSigGreenTRCTag);
    Shapes[2] = (cmsToneCurve *) cmsReadTag(hProfile, cmsSigBlueTRCTag);

    if (!Shapes[0] || !Shapes[1] || !Shapes[2])
        return NULL;

    Lut = cmsPipelineAlloc(ContextID, 3, 3);
    if (Lut != NULL) {
     
        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, 3, Shapes));
        cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocMatrix(ContextID, 3, 3, (cmsFloat64Number*) &Mat, NULL));
        
        // Note that it is certainly possible a single profile would have a LUT based
        // tag for output working in lab and a matrix-shaper for the fallback cases. 
        // This is not allowed by the spec, but this code is tolerant to those cases    
        if (cmsGetPCS(hProfile) == cmsSigLabData) {

             cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocXYZ2Lab(ContextID));
        }

    }

    return Lut;
}