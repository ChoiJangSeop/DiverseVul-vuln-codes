cmsPipeline* DefaultICCintents(cmsContext       ContextID,
                               cmsUInt32Number  nProfiles,
                               cmsUInt32Number  TheIntents[],
                               cmsHPROFILE      hProfiles[],
                               cmsBool          BPC[],
                               cmsFloat64Number AdaptationStates[],
                               cmsUInt32Number  dwFlags)
{
    cmsPipeline* Lut = NULL;
    cmsPipeline* Result;
    cmsHPROFILE hProfile;
    cmsMAT3 m;
    cmsVEC3 off;
    cmsColorSpaceSignature ColorSpaceIn, ColorSpaceOut, CurrentColorSpace;
    cmsProfileClassSignature ClassSig;
    cmsUInt32Number  i, Intent;

    // For safety
    if (nProfiles == 0) return NULL;

    // Allocate an empty LUT for holding the result. 0 as channel count means 'undefined'
    Result = cmsPipelineAlloc(ContextID, 0, 0);
    if (Result == NULL) return NULL;

    CurrentColorSpace = cmsGetColorSpace(hProfiles[0]);

    for (i=0; i < nProfiles; i++) {

        cmsBool  lIsDeviceLink, lIsInput;

        hProfile      = hProfiles[i];
        ClassSig      = cmsGetDeviceClass(hProfile);
        lIsDeviceLink = (ClassSig == cmsSigLinkClass || ClassSig == cmsSigAbstractClass );

        // First profile is used as input unless devicelink or abstract
        if ((i == 0) && !lIsDeviceLink) {
            lIsInput = TRUE;
        }
        else {
          // Else use profile in the input direction if current space is not PCS
        lIsInput      = (CurrentColorSpace != cmsSigXYZData) &&
                        (CurrentColorSpace != cmsSigLabData);
        }

        Intent        = TheIntents[i];

        if (lIsInput || lIsDeviceLink) {

            ColorSpaceIn    = cmsGetColorSpace(hProfile);
            ColorSpaceOut   = cmsGetPCS(hProfile);
        }
        else {

            ColorSpaceIn    = cmsGetPCS(hProfile);
            ColorSpaceOut   = cmsGetColorSpace(hProfile);
        }

        if (!ColorSpaceIsCompatible(ColorSpaceIn, CurrentColorSpace)) {

            cmsSignalError(ContextID, cmsERROR_COLORSPACE_CHECK, "ColorSpace mismatch");
            goto Error;
        }

        // If devicelink is found, then no custom intent is allowed and we can
        // read the LUT to be applied. Settings don't apply here.
        if (lIsDeviceLink || ((ClassSig == cmsSigNamedColorClass) && (nProfiles == 1))) {

            // Get the involved LUT from the profile
            Lut = _cmsReadDevicelinkLUT(hProfile, Intent);
            if (Lut == NULL) goto Error;

            // What about abstract profiles?
             if (ClassSig == cmsSigAbstractClass && i > 0) {
                if (!ComputeConversion(i, hProfiles, Intent, BPC[i], AdaptationStates[i], &m, &off)) goto Error;
             }
             else {
                _cmsMAT3identity(&m);
                _cmsVEC3init(&off, 0, 0, 0);
             }


            if (!AddConversion(Result, CurrentColorSpace, ColorSpaceIn, &m, &off)) goto Error;

        }
        else {

            if (lIsInput) {
                // Input direction means non-pcs connection, so proceed like devicelinks
                Lut = _cmsReadInputLUT(hProfile, Intent);
                if (Lut == NULL) goto Error;
            }
            else {

                // Output direction means PCS connection. Intent may apply here
                Lut = _cmsReadOutputLUT(hProfile, Intent);
                if (Lut == NULL) goto Error;


                if (!ComputeConversion(i, hProfiles, Intent, BPC[i], AdaptationStates[i], &m, &off)) goto Error;
                if (!AddConversion(Result, CurrentColorSpace, ColorSpaceIn, &m, &off)) goto Error;

            }
        }

        // Concatenate to the output LUT
        if (!cmsPipelineCat(Result, Lut))
            goto Error;
        cmsPipelineFree(Lut);

        // Update current space
        CurrentColorSpace = ColorSpaceOut;
    }

    return Result;

Error:

    cmsPipelineFree(Lut);
    if (Result != NULL) cmsPipelineFree(Result);
    return NULL;

    cmsUNUSED_PARAMETER(dwFlags);
}