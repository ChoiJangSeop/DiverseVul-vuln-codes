void CMSEXPORT cmsPipelineInsertStage(cmsPipeline* lut, cmsStageLoc loc, cmsStage* mpe)
{
    cmsStage* Anterior = NULL, *pt;

    _cmsAssert(lut != NULL);
    _cmsAssert(mpe != NULL);

    switch (loc) {

        case cmsAT_BEGIN:
            mpe ->Next = lut ->Elements;
            lut ->Elements = mpe;
            break;

        case cmsAT_END:

            if (lut ->Elements == NULL)
                lut ->Elements = mpe;
            else {

                for (pt = lut ->Elements;
                     pt != NULL;
                     pt = pt -> Next) Anterior = pt;

                Anterior ->Next = mpe;
                mpe ->Next = NULL;
            }
            break;
        default:;
    }

    BlessLUT(lut);
}