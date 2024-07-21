void* Type_LUTB2A_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsUInt32Number* nItems, cmsUInt32Number SizeOfTag)
{
    cmsUInt8Number       inputChan;      // Number of input channels
    cmsUInt8Number       outputChan;     // Number of output channels
    cmsUInt32Number      BaseOffset;     // Actual position in file
    cmsUInt32Number      offsetB;        // Offset to first "B" curve
    cmsUInt32Number      offsetMat;      // Offset to matrix
    cmsUInt32Number      offsetM;        // Offset to first "M" curve
    cmsUInt32Number      offsetC;        // Offset to CLUT
    cmsUInt32Number      offsetA;        // Offset to first "A" curve
    cmsStage* mpe;
    cmsPipeline* NewLUT = NULL;


    BaseOffset = io ->Tell(io) - sizeof(_cmsTagBase);

    if (!_cmsReadUInt8Number(io, &inputChan)) return NULL;
    if (!_cmsReadUInt8Number(io, &outputChan)) return NULL;

    // Padding
    if (!_cmsReadUInt16Number(io, NULL)) return NULL;

    if (!_cmsReadUInt32Number(io, &offsetB)) return NULL;
    if (!_cmsReadUInt32Number(io, &offsetMat)) return NULL;
    if (!_cmsReadUInt32Number(io, &offsetM)) return NULL;
    if (!_cmsReadUInt32Number(io, &offsetC)) return NULL;
    if (!_cmsReadUInt32Number(io, &offsetA)) return NULL;

    // Allocates an empty LUT
    NewLUT = cmsPipelineAlloc(self ->ContextID, inputChan, outputChan);
    if (NewLUT == NULL) return NULL;

    if (offsetB != 0) {
        mpe = ReadSetOfCurves(self, io, BaseOffset + offsetB, inputChan);
        if (mpe == NULL) { cmsPipelineFree(NewLUT); return NULL; }
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    if (offsetMat != 0) {
        mpe = ReadMatrix(self, io, BaseOffset + offsetMat);
        if (mpe == NULL) { cmsPipelineFree(NewLUT); return NULL; }
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    if (offsetM != 0) {
        mpe = ReadSetOfCurves(self, io, BaseOffset + offsetM, inputChan);
        if (mpe == NULL) { cmsPipelineFree(NewLUT); return NULL; }
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    if (offsetC != 0) {
        mpe = ReadCLUT(self, io, BaseOffset + offsetC, inputChan, outputChan);
        if (mpe == NULL) { cmsPipelineFree(NewLUT); return NULL; }
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    if (offsetA!= 0) {
        mpe = ReadSetOfCurves(self, io, BaseOffset + offsetA, outputChan);
        if (mpe == NULL) { cmsPipelineFree(NewLUT); return NULL; }
        cmsPipelineInsertStage(NewLUT, cmsAT_END, mpe);
    }

    *nItems = 1;
    return NewLUT;

    cmsUNUSED_PARAMETER(SizeOfTag);
}