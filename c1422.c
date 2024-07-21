cmsBool OptimizeMatrixShaper(cmsPipeline** Lut, cmsUInt32Number Intent, cmsUInt32Number* InputFormat, cmsUInt32Number* OutputFormat, cmsUInt32Number* dwFlags)
{
    cmsStage* Curve1, *Curve2;
    cmsStage* Matrix1, *Matrix2;
    _cmsStageMatrixData* Data1;
    _cmsStageMatrixData* Data2;
    cmsMAT3 res;
    cmsBool IdentityMat;
    cmsPipeline* Dest, *Src;

    // Only works on RGB to RGB
    if (T_CHANNELS(*InputFormat) != 3 || T_CHANNELS(*OutputFormat) != 3) return FALSE;

    // Only works on 8 bit input
    if (!_cmsFormatterIs8bit(*InputFormat)) return FALSE;

    // Seems suitable, proceed
    Src = *Lut;

    // Check for shaper-matrix-matrix-shaper structure, that is what this optimizer stands for
    if (!cmsPipelineCheckAndRetreiveStages(Src, 4,
        cmsSigCurveSetElemType, cmsSigMatrixElemType, cmsSigMatrixElemType, cmsSigCurveSetElemType,
        &Curve1, &Matrix1, &Matrix2, &Curve2)) return FALSE;

    // Get both matrices
    Data1 = (_cmsStageMatrixData*) cmsStageData(Matrix1);
    Data2 = (_cmsStageMatrixData*) cmsStageData(Matrix2);

    // Input offset should be zero
    if (Data1 ->Offset != NULL) return FALSE;

    // Multiply both matrices to get the result
    _cmsMAT3per(&res, (cmsMAT3*) Data2 ->Double, (cmsMAT3*) Data1 ->Double);

    // Now the result is in res + Data2 -> Offset. Maybe is a plain identity?
    IdentityMat = FALSE;
    if (_cmsMAT3isIdentity(&res) && Data2 ->Offset == NULL) {

        // We can get rid of full matrix
        IdentityMat = TRUE;
    }

      // Allocate an empty LUT
    Dest =  cmsPipelineAlloc(Src ->ContextID, Src ->InputChannels, Src ->OutputChannels);
    if (!Dest) return FALSE;

    // Assamble the new LUT
    cmsPipelineInsertStage(Dest, cmsAT_BEGIN, cmsStageDup(Curve1));
    if (!IdentityMat)
        cmsPipelineInsertStage(Dest, cmsAT_END, cmsStageAllocMatrix(Dest ->ContextID, 3, 3, (const cmsFloat64Number*) &res, Data2 ->Offset));
    cmsPipelineInsertStage(Dest, cmsAT_END, cmsStageDup(Curve2));

    // If identity on matrix, we can further optimize the curves, so call the join curves routine
    if (IdentityMat) {

        OptimizeByJoiningCurves(&Dest, Intent, InputFormat, OutputFormat, dwFlags);
    }
    else {
        _cmsStageToneCurvesData* mpeC1 = (_cmsStageToneCurvesData*) cmsStageData(Curve1);
        _cmsStageToneCurvesData* mpeC2 = (_cmsStageToneCurvesData*) cmsStageData(Curve2);

        // In this particular optimization, cach does not help as it takes more time to deal with
        // the cach that with the pixel handling
        *dwFlags |= cmsFLAGS_NOCACHE;

        // Setup the optimizarion routines
        SetMatShaper(Dest, mpeC1 ->TheCurves, &res, (cmsVEC3*) Data2 ->Offset, mpeC2->TheCurves, OutputFormat);
    }

    cmsPipelineFree(Src);
    *Lut = Dest;
    return TRUE;
}