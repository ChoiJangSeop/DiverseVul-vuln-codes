cmsBool CMSEXPORT cmsStageSampleCLutFloat(cmsStage* mpe, cmsSAMPLERFLOAT Sampler, void * Cargo, cmsUInt32Number dwFlags)
{
    int i, t, nTotalPoints, index, rest;
    int nInputs, nOutputs;
    cmsUInt32Number* nSamples;
    cmsFloat32Number In[cmsMAXCHANNELS], Out[MAX_STAGE_CHANNELS];
    _cmsStageCLutData* clut = (_cmsStageCLutData*) mpe->Data;

    nSamples = clut->Params ->nSamples;
    nInputs  = clut->Params ->nInputs;
    nOutputs = clut->Params ->nOutputs;

    if (nInputs >= cmsMAXCHANNELS) return FALSE;
    if (nOutputs >= MAX_STAGE_CHANNELS) return FALSE;

    nTotalPoints = CubeSize(nSamples, nInputs);
    if (nTotalPoints == 0) return FALSE;

    index = 0;
    for (i = 0; i < nTotalPoints; i++) {

        rest = i;
        for (t = nInputs-1; t >=0; --t) {

            cmsUInt32Number  Colorant = rest % nSamples[t];

            rest /= nSamples[t];

            In[t] =  (cmsFloat32Number) (_cmsQuantizeVal(Colorant, nSamples[t]) / 65535.0);
        }

        if (clut ->Tab.TFloat != NULL) {
            for (t=0; t < nOutputs; t++)
                Out[t] = clut->Tab.TFloat[index + t];
        }

        if (!Sampler(In, Out, Cargo))
            return FALSE;

        if (!(dwFlags & SAMPLER_INSPECT)) {

            if (clut ->Tab.TFloat != NULL) {
                for (t=0; t < nOutputs; t++)
                    clut->Tab.TFloat[index + t] = Out[t];
            }
        }

        index += nOutputs;
    }

    return TRUE;
}