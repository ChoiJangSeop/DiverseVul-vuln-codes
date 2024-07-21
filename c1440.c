cmsStage* _cmsStageAllocLabV2ToV4curves(cmsContext ContextID)
{
    cmsStage* mpe;
    cmsToneCurve* LabTable[3];
    int i, j;

    LabTable[0] = cmsBuildTabulatedToneCurve16(ContextID, 258, NULL);
    LabTable[1] = cmsBuildTabulatedToneCurve16(ContextID, 258, NULL);
    LabTable[2] = cmsBuildTabulatedToneCurve16(ContextID, 258, NULL);

    for (j=0; j < 3; j++) {

        if (LabTable[j] == NULL) {
            cmsFreeToneCurveTriple(LabTable);
            return NULL;
        }

        // We need to map * (0xffff / 0xff00), thats same as (257 / 256)
        // So we can use 258-entry tables to do the trick (i / 257) * (255 * 257) * (257 / 256);
        for (i=0; i < 257; i++)  {

            LabTable[j]->Table16[i] = (cmsUInt16Number) ((i * 0xffff + 0x80) >> 8);
        }

        LabTable[j] ->Table16[257] = 0xffff;
    }

    mpe = cmsStageAllocToneCurves(ContextID, 3, LabTable);
    cmsFreeToneCurveTriple(LabTable);

    mpe ->Implements = cmsSigLabV2toV4;
    return mpe;
}