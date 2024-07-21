cmsPipeline* CMSEXPORT cmsPipelineDup(const cmsPipeline* lut)
{
    cmsPipeline* NewLUT;
    cmsStage *NewMPE, *Anterior = NULL, *mpe;
    cmsBool  First = TRUE;

    if (lut == NULL) return NULL;

    NewLUT = cmsPipelineAlloc(lut ->ContextID, lut ->InputChannels, lut ->OutputChannels);
    for (mpe = lut ->Elements;
         mpe != NULL;
         mpe = mpe ->Next) {

             NewMPE = cmsStageDup(mpe);

             if (NewMPE == NULL) {
                 cmsPipelineFree(NewLUT);
                 return NULL;
             }

             if (First) {
                 NewLUT ->Elements = NewMPE;
                 First = FALSE;
             }
             else {
                Anterior ->Next = NewMPE;
             }

            Anterior = NewMPE;
    }

    NewLUT ->Eval16Fn    = lut ->Eval16Fn;
    NewLUT ->EvalFloatFn = lut ->EvalFloatFn;
    NewLUT ->DupDataFn   = lut ->DupDataFn;
    NewLUT ->FreeDataFn  = lut ->FreeDataFn;

    if (NewLUT ->DupDataFn != NULL)
        NewLUT ->Data = NewLUT ->DupDataFn(lut ->ContextID, lut->Data);


    NewLUT ->SaveAs8Bits    = lut ->SaveAs8Bits;

    BlessLUT(NewLUT);
    return NewLUT;
}