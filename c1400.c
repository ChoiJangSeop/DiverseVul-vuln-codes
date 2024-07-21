cmsBool OptimizeByResampling(cmsPipeline** Lut, cmsUInt32Number Intent, cmsUInt32Number* InputFormat, cmsUInt32Number* OutputFormat, cmsUInt32Number* dwFlags)
{
    cmsPipeline* Src;
    cmsPipeline* Dest;
    cmsStage* mpe;
    cmsStage* CLUT;
    cmsStage *KeepPreLin = NULL, *KeepPostLin = NULL;
    int nGridPoints;
    cmsColorSpaceSignature ColorSpace, OutputColorSpace;
    cmsStage *NewPreLin = NULL;
    cmsStage *NewPostLin = NULL;
    _cmsStageCLutData* DataCLUT;
    cmsToneCurve** DataSetIn;
    cmsToneCurve** DataSetOut;
    Prelin16Data* p16;


    // This is a loosy optimization! does not apply in floating-point cases
    if (_cmsFormatterIsFloat(*InputFormat) || _cmsFormatterIsFloat(*OutputFormat)) return FALSE;

    ColorSpace       = _cmsICCcolorSpace(T_COLORSPACE(*InputFormat));
    OutputColorSpace = _cmsICCcolorSpace(T_COLORSPACE(*OutputFormat));
    nGridPoints      = _cmsReasonableGridpointsByColorspace(ColorSpace, *dwFlags);

    // For empty LUTs, 2 points are enough
    if (cmsPipelineStageCount(*Lut) == 0)
        nGridPoints = 2;

    Src = *Lut;

   // Named color pipelines cannot be optimized either
   for (mpe = cmsPipelineGetPtrToFirstStage(Src);
         mpe != NULL;
         mpe = cmsStageNext(mpe)) {
            if (cmsStageType(mpe) == cmsSigNamedColorElemType) return FALSE;
    }

    // Allocate an empty LUT
    Dest =  cmsPipelineAlloc(Src ->ContextID, Src ->InputChannels, Src ->OutputChannels);
    if (!Dest) return FALSE;

    // Prelinearization tables are kept unless indicated by flags
    if (*dwFlags & cmsFLAGS_CLUT_PRE_LINEARIZATION) {

        // Get a pointer to the prelinearization element
        cmsStage* PreLin = cmsPipelineGetPtrToFirstStage(Src);

        // Check if suitable
        if (PreLin ->Type == cmsSigCurveSetElemType) {

            // Maybe this is a linear tram, so we can avoid the whole stuff
            if (!AllCurvesAreLinear(PreLin)) {

                // All seems ok, proceed.
                NewPreLin = cmsStageDup(PreLin);
                cmsPipelineInsertStage(Dest, cmsAT_BEGIN, NewPreLin);

                // Remove prelinearization. Since we have duplicated the curve
                // in destination LUT, the sampling shoud be applied after this stage.
                cmsPipelineUnlinkStage(Src, cmsAT_BEGIN, &KeepPreLin);
            }
        }
    }

    // Allocate the CLUT
    CLUT = cmsStageAllocCLut16bit(Src ->ContextID, nGridPoints, Src ->InputChannels, Src->OutputChannels, NULL);
    if (CLUT == NULL) return FALSE;

    // Add the CLUT to the destination LUT
    cmsPipelineInsertStage(Dest, cmsAT_END, CLUT);

    // Postlinearization tables are kept unless indicated by flags
    if (*dwFlags & cmsFLAGS_CLUT_POST_LINEARIZATION) {

        // Get a pointer to the postlinearization if present
        cmsStage* PostLin = cmsPipelineGetPtrToLastStage(Src);

        // Check if suitable
        if (cmsStageType(PostLin) == cmsSigCurveSetElemType) {

            // Maybe this is a linear tram, so we can avoid the whole stuff
            if (!AllCurvesAreLinear(PostLin)) {

                // All seems ok, proceed.
                NewPostLin = cmsStageDup(PostLin);
                cmsPipelineInsertStage(Dest, cmsAT_END, NewPostLin);

                // In destination LUT, the sampling shoud be applied after this stage.
                cmsPipelineUnlinkStage(Src, cmsAT_END, &KeepPostLin);
            }
        }
    }

    // Now its time to do the sampling. We have to ignore pre/post linearization
    // The source LUT whithout pre/post curves is passed as parameter.
    if (!cmsStageSampleCLut16bit(CLUT, XFormSampler16, (void*) Src, 0)) {

        // Ops, something went wrong, Restore stages
        if (KeepPreLin != NULL)  cmsPipelineInsertStage(Src, cmsAT_BEGIN, KeepPreLin);
        if (KeepPostLin != NULL) cmsPipelineInsertStage(Src, cmsAT_END,   KeepPostLin);
        cmsPipelineFree(Dest);
        return FALSE;
    }

    // Done.

    if (KeepPreLin != NULL) cmsStageFree(KeepPreLin);
    if (KeepPostLin != NULL) cmsStageFree(KeepPostLin);
    cmsPipelineFree(Src);

    DataCLUT = (_cmsStageCLutData*) CLUT ->Data;

    if (NewPreLin == NULL) DataSetIn = NULL;
    else DataSetIn = ((_cmsStageToneCurvesData*) NewPreLin ->Data) ->TheCurves;

    if (NewPostLin == NULL) DataSetOut = NULL;
    else  DataSetOut = ((_cmsStageToneCurvesData*) NewPostLin ->Data) ->TheCurves;


    if (DataSetIn == NULL && DataSetOut == NULL) {

        _cmsPipelineSetOptimizationParameters(Dest, (_cmsOPTeval16Fn) DataCLUT->Params->Interpolation.Lerp16, DataCLUT->Params, NULL, NULL);
    }
    else {

        p16 = PrelinOpt16alloc(Dest ->ContextID,
                               DataCLUT ->Params,
                               Dest ->InputChannels,
                               DataSetIn,
                               Dest ->OutputChannels,
                               DataSetOut);


        _cmsPipelineSetOptimizationParameters(Dest, PrelinEval16, (void*) p16, PrelinOpt16free, Prelin16dup);
    }


    // Don't fix white on absolute colorimetric
    if (Intent == INTENT_ABSOLUTE_COLORIMETRIC)
        *dwFlags |= cmsFLAGS_NOWHITEONWHITEFIXUP;

    if (!(*dwFlags & cmsFLAGS_NOWHITEONWHITEFIXUP)) {

        FixWhiteMisalignment(Dest, ColorSpace, OutputColorSpace);
    }

    *Lut = Dest;
    return TRUE;

    cmsUNUSED_PARAMETER(Intent);
}