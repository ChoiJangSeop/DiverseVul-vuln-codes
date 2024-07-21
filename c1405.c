cmsBool AddConversion(cmsPipeline* Result, cmsColorSpaceSignature InPCS, cmsColorSpaceSignature OutPCS, cmsMAT3* m, cmsVEC3* off)
{
    cmsFloat64Number* m_as_dbl = (cmsFloat64Number*) m;
    cmsFloat64Number* off_as_dbl = (cmsFloat64Number*) off;

    // Handle PCS mismatches. A specialized stage is added to the LUT in such case
    switch (InPCS) {

        case cmsSigXYZData: // Input profile operates in XYZ

            switch (OutPCS) {

            case cmsSigXYZData:  // XYZ -> XYZ
                if (!IsEmptyLayer(m, off))
                    cmsPipelineInsertStage(Result, cmsAT_END, cmsStageAllocMatrix(Result ->ContextID, 3, 3, m_as_dbl, off_as_dbl));
                break;

            case cmsSigLabData:  // XYZ -> Lab
                if (!IsEmptyLayer(m, off))
                    cmsPipelineInsertStage(Result, cmsAT_END, cmsStageAllocMatrix(Result ->ContextID, 3, 3, m_as_dbl, off_as_dbl));
                cmsPipelineInsertStage(Result, cmsAT_END, _cmsStageAllocXYZ2Lab(Result ->ContextID));
                break;

            default:
                return FALSE;   // Colorspace mismatch
                }
                break;


        case cmsSigLabData: // Input profile operates in Lab

            switch (OutPCS) {

            case cmsSigXYZData:  // Lab -> XYZ

                cmsPipelineInsertStage(Result, cmsAT_END, _cmsStageAllocLab2XYZ(Result ->ContextID));
                if (!IsEmptyLayer(m, off))
                    cmsPipelineInsertStage(Result, cmsAT_END, cmsStageAllocMatrix(Result ->ContextID, 3, 3, m_as_dbl, off_as_dbl));
                break;

            case cmsSigLabData:  // Lab -> Lab

                if (!IsEmptyLayer(m, off)) {
                    cmsPipelineInsertStage(Result, cmsAT_END, _cmsStageAllocLab2XYZ(Result ->ContextID));
                    cmsPipelineInsertStage(Result, cmsAT_END, cmsStageAllocMatrix(Result ->ContextID, 3, 3, m_as_dbl, off_as_dbl));
                    cmsPipelineInsertStage(Result, cmsAT_END, _cmsStageAllocXYZ2Lab(Result ->ContextID));
                }
                break;

            default:
                return FALSE;  // Mismatch
            }
            break;


            // On colorspaces other than PCS, check for same space
        default:
            if (InPCS != OutPCS) return FALSE;
            break;
    }

    return TRUE;
}