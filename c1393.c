cmsBool OptimizeByComputingLinearization(cmsPipeline** Lut, cmsUInt32Number Intent, cmsUInt32Number* InputFormat, cmsUInt32Number* OutputFormat, cmsUInt32Number* dwFlags)
{
    cmsPipeline* OriginalLut;
    int nGridPoints;
    cmsToneCurve *Trans[cmsMAXCHANNELS], *TransReverse[cmsMAXCHANNELS];
    cmsUInt32Number t, i;
    cmsFloat32Number v, In[cmsMAXCHANNELS], Out[cmsMAXCHANNELS];
    cmsBool lIsSuitable, lIsLinear;
    cmsPipeline* OptimizedLUT = NULL, *LutPlusCurves = NULL;
    cmsStage* OptimizedCLUTmpe;
    cmsColorSpaceSignature ColorSpace, OutputColorSpace;
    cmsStage* OptimizedPrelinMpe;
    cmsStage* mpe;
    cmsToneCurve**   OptimizedPrelinCurves;
    _cmsStageCLutData*     OptimizedPrelinCLUT;


    // This is a loosy optimization! does not apply in floating-point cases
    if (_cmsFormatterIsFloat(*InputFormat) || _cmsFormatterIsFloat(*OutputFormat)) return FALSE;

    // Only on RGB
    if (T_COLORSPACE(*InputFormat)  != PT_RGB) return FALSE;
    if (T_COLORSPACE(*OutputFormat) != PT_RGB) return FALSE;


    // On 16 bits, user has to specify the feature
    if (!_cmsFormatterIs8bit(*InputFormat)) {
        if (!(*dwFlags & cmsFLAGS_CLUT_PRE_LINEARIZATION)) return FALSE;
    }

    OriginalLut = *Lut;

   // Named color pipelines cannot be optimized either
   for (mpe = cmsPipelineGetPtrToFirstStage(OriginalLut);
         mpe != NULL;
         mpe = cmsStageNext(mpe)) {
            if (cmsStageType(mpe) == cmsSigNamedColorElemType) return FALSE;
    }

    ColorSpace       = _cmsICCcolorSpace(T_COLORSPACE(*InputFormat));
    OutputColorSpace = _cmsICCcolorSpace(T_COLORSPACE(*OutputFormat));
    nGridPoints      = _cmsReasonableGridpointsByColorspace(ColorSpace, *dwFlags);

    // Empty gamma containers
    memset(Trans, 0, sizeof(Trans));
    memset(TransReverse, 0, sizeof(TransReverse));

    for (t = 0; t < OriginalLut ->InputChannels; t++) {
        Trans[t] = cmsBuildTabulatedToneCurve16(OriginalLut ->ContextID, PRELINEARIZATION_POINTS, NULL);
        if (Trans[t] == NULL) goto Error;
    }

    // Populate the curves
    for (i=0; i < PRELINEARIZATION_POINTS; i++) {

        v = (cmsFloat32Number) ((cmsFloat64Number) i / (PRELINEARIZATION_POINTS - 1));

        // Feed input with a gray ramp
        for (t=0; t < OriginalLut ->InputChannels; t++)
            In[t] = v;

        // Evaluate the gray value
        cmsPipelineEvalFloat(In, Out, OriginalLut);

        // Store result in curve
        for (t=0; t < OriginalLut ->InputChannels; t++)
            Trans[t] ->Table16[i] = _cmsQuickSaturateWord(Out[t] * 65535.0);
    }

    // Slope-limit the obtained curves
    for (t = 0; t < OriginalLut ->InputChannels; t++)
        SlopeLimiting(Trans[t]);

    // Check for validity
    lIsSuitable = TRUE;
    lIsLinear   = TRUE;
    for (t=0; (lIsSuitable && (t < OriginalLut ->InputChannels)); t++) {

        // Exclude if already linear
        if (!cmsIsToneCurveLinear(Trans[t]))
            lIsLinear = FALSE;

        // Exclude if non-monotonic
        if (!cmsIsToneCurveMonotonic(Trans[t]))
            lIsSuitable = FALSE;

        if (IsDegenerated(Trans[t]))
            lIsSuitable = FALSE;
    }

    // If it is not suitable, just quit
    if (!lIsSuitable) goto Error;

    // Invert curves if possible
    for (t = 0; t < OriginalLut ->InputChannels; t++) {
        TransReverse[t] = cmsReverseToneCurveEx(PRELINEARIZATION_POINTS, Trans[t]);
        if (TransReverse[t] == NULL) goto Error;
    }

    // Now inset the reversed curves at the begin of transform
    LutPlusCurves = cmsPipelineDup(OriginalLut);
    if (LutPlusCurves == NULL) goto Error;

    cmsPipelineInsertStage(LutPlusCurves, cmsAT_BEGIN, cmsStageAllocToneCurves(OriginalLut ->ContextID, OriginalLut ->InputChannels, TransReverse));

    // Create the result LUT
    OptimizedLUT = cmsPipelineAlloc(OriginalLut ->ContextID, OriginalLut ->InputChannels, OriginalLut ->OutputChannels);
    if (OptimizedLUT == NULL) goto Error;

    OptimizedPrelinMpe = cmsStageAllocToneCurves(OriginalLut ->ContextID, OriginalLut ->InputChannels, Trans);

    // Create and insert the curves at the beginning
    cmsPipelineInsertStage(OptimizedLUT, cmsAT_BEGIN, OptimizedPrelinMpe);

    // Allocate the CLUT for result
    OptimizedCLUTmpe = cmsStageAllocCLut16bit(OriginalLut ->ContextID, nGridPoints, OriginalLut ->InputChannels, OriginalLut ->OutputChannels, NULL);

    // Add the CLUT to the destination LUT
    cmsPipelineInsertStage(OptimizedLUT, cmsAT_END, OptimizedCLUTmpe);

    // Resample the LUT
    if (!cmsStageSampleCLut16bit(OptimizedCLUTmpe, XFormSampler16, (void*) LutPlusCurves, 0)) goto Error;

    // Free resources
    for (t = 0; t < OriginalLut ->InputChannels; t++) {

        if (Trans[t]) cmsFreeToneCurve(Trans[t]);
        if (TransReverse[t]) cmsFreeToneCurve(TransReverse[t]);
    }

    cmsPipelineFree(LutPlusCurves);


    OptimizedPrelinCurves = _cmsStageGetPtrToCurveSet(OptimizedPrelinMpe);
    OptimizedPrelinCLUT   = (_cmsStageCLutData*) OptimizedCLUTmpe ->Data;

    // Set the evaluator if 8-bit
    if (_cmsFormatterIs8bit(*InputFormat)) {

        Prelin8Data* p8 = PrelinOpt8alloc(OptimizedLUT ->ContextID,
                                                OptimizedPrelinCLUT ->Params,
                                                OptimizedPrelinCurves);
        if (p8 == NULL) return FALSE;

        _cmsPipelineSetOptimizationParameters(OptimizedLUT, PrelinEval8, (void*) p8, Prelin8free, Prelin8dup);

    }
    else
    {
        Prelin16Data* p16 = PrelinOpt16alloc(OptimizedLUT ->ContextID,
            OptimizedPrelinCLUT ->Params,
            3, OptimizedPrelinCurves, 3, NULL);
        if (p16 == NULL) return FALSE;

        _cmsPipelineSetOptimizationParameters(OptimizedLUT, PrelinEval16, (void*) p16, PrelinOpt16free, Prelin16dup);

    }

    // Don't fix white on absolute colorimetric
    if (Intent == INTENT_ABSOLUTE_COLORIMETRIC)
        *dwFlags |= cmsFLAGS_NOWHITEONWHITEFIXUP;

    if (!(*dwFlags & cmsFLAGS_NOWHITEONWHITEFIXUP)) {

        if (!FixWhiteMisalignment(OptimizedLUT, ColorSpace, OutputColorSpace)) {

            return FALSE;
        }
    }

    // And return the obtained LUT

    cmsPipelineFree(OriginalLut);
    *Lut = OptimizedLUT;
    return TRUE;

Error:

    for (t = 0; t < OriginalLut ->InputChannels; t++) {

        if (Trans[t]) cmsFreeToneCurve(Trans[t]);
        if (TransReverse[t]) cmsFreeToneCurve(TransReverse[t]);
    }

    if (LutPlusCurves != NULL) cmsPipelineFree(LutPlusCurves);
    if (OptimizedLUT != NULL) cmsPipelineFree(OptimizedLUT);

    return FALSE;

    cmsUNUSED_PARAMETER(Intent);
}