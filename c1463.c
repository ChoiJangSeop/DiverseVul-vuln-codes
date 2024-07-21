cmsStage* CMSEXPORT cmsStageAllocCLut16bitGranular(cmsContext ContextID,
                                         const cmsUInt32Number clutPoints[],
                                         cmsUInt32Number inputChan,
                                         cmsUInt32Number outputChan,
                                         const cmsUInt16Number* Table)
{
    cmsUInt32Number i, n;
    _cmsStageCLutData* NewElem;
    cmsStage* NewMPE;

    NewMPE = _cmsStageAllocPlaceholder(ContextID, cmsSigCLutElemType, inputChan, outputChan,
                                     EvaluateCLUTfloatIn16, CLUTElemDup, CLutElemTypeFree, NULL );

    if (NewMPE == NULL) return NULL;

    NewElem = (_cmsStageCLutData*) _cmsMallocZero(ContextID, sizeof(_cmsStageCLutData));
    if (NewElem == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    NewMPE ->Data  = (void*) NewElem;

    NewElem -> nEntries = n = outputChan * CubeSize(clutPoints, inputChan);
    NewElem -> HasFloatValues = FALSE;

    if (n == 0) {
        cmsStageFree(NewMPE);
        return NULL;
    }


    NewElem ->Tab.T  = (cmsUInt16Number*) _cmsCalloc(ContextID, n, sizeof(cmsUInt16Number));
    if (NewElem ->Tab.T == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    if (Table != NULL) {
        for (i=0; i < n; i++) {
            NewElem ->Tab.T[i] = Table[i];
        }
    }

    NewElem ->Params = _cmsComputeInterpParamsEx(ContextID, clutPoints, inputChan, outputChan, NewElem ->Tab.T, CMS_LERP_FLAGS_16BITS);
    if (NewElem ->Params == NULL) {
        cmsStageFree(NewMPE);
        return NULL;
    }

    return NewMPE;
}