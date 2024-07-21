cmsStage* CMSEXPORT cmsStageAllocCLutFloatGranular(cmsContext ContextID, const cmsUInt32Number clutPoints[], cmsUInt32Number inputChan, cmsUInt32Number outputChan, const cmsFloat32Number* Table)
{
    cmsUInt32Number i, n;
    _cmsStageCLutData* NewElem;
    cmsStage* NewMPE;

    _cmsAssert(clutPoints != NULL);

    if (inputChan > MAX_INPUT_DIMENSIONS) {
        cmsSignalError(ContextID, cmsERROR_RANGE, "Too many input channels (%d channels, max=%d)", inputChan, MAX_INPUT_DIMENSIONS);
        return NULL;
    }

    NewMPE = _cmsStageAllocPlaceholder(ContextID, cmsSigCLutElemType, inputChan, outputChan,
                                             EvaluateCLUTfloat, CLUTElemDup, CLutElemTypeFree, NULL);
    if (NewMPE == NULL) return NULL;


    NewElem = (_cmsStageCLutData*) _cmsMallocZero(ContextID, sizeof(_cmsStageCLutData));
    if (NewElem == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    NewMPE ->Data  = (void*) NewElem;

    // There is a potential integer overflow on conputing n and nEntries.
    NewElem -> nEntries = n = outputChan * CubeSize(clutPoints, inputChan);
    NewElem -> HasFloatValues = TRUE;

    if (n == 0) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    NewElem ->Tab.TFloat  = (cmsFloat32Number*) _cmsCalloc(ContextID, n, sizeof(cmsFloat32Number));
    if (NewElem ->Tab.TFloat == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    if (Table != NULL) {
        for (i=0; i < n; i++) {
            NewElem ->Tab.TFloat[i] = Table[i];
        }
    }


    NewElem ->Params = _cmsComputeInterpParamsEx(ContextID, clutPoints,  inputChan, outputChan, NewElem ->Tab.TFloat, CMS_LERP_FLAGS_FLOAT);
    if (NewElem ->Params == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }



    return NewMPE;
}