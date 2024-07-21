cmsBool CMSEXPORT cmsDetectBlackPoint(cmsCIEXYZ* BlackPoint, cmsHPROFILE hProfile, cmsUInt32Number Intent, cmsUInt32Number dwFlags)
{

    // Zero for black point
    if (cmsGetDeviceClass(hProfile) == cmsSigLinkClass) {

        BlackPoint -> X = BlackPoint ->Y = BlackPoint -> Z = 0.0;
        return FALSE;
    }

    // v4 + perceptual & saturation intents does have its own black point, and it is
    // well specified enough to use it. Black point tag is deprecated in V4.

    if ((cmsGetEncodedICCversion(hProfile) >= 0x4000000) &&
        (Intent == INTENT_PERCEPTUAL || Intent == INTENT_SATURATION)) {

            // Matrix shaper share MRC & perceptual intents
            if (cmsIsMatrixShaper(hProfile))
                return BlackPointAsDarkerColorant(hProfile, INTENT_RELATIVE_COLORIMETRIC, BlackPoint, 0);

            // Get Perceptual black out of v4 profiles. That is fixed for perceptual & saturation intents
            BlackPoint -> X = cmsPERCEPTUAL_BLACK_X;
            BlackPoint -> Y = cmsPERCEPTUAL_BLACK_Y;
            BlackPoint -> Z = cmsPERCEPTUAL_BLACK_Z;

            return TRUE;
    }


#ifdef CMS_USE_PROFILE_BLACK_POINT_TAG

    // v2, v4 rel/abs colorimetric
    if (cmsIsTag(hProfile, cmsSigMediaBlackPointTag) &&
        Intent == INTENT_RELATIVE_COLORIMETRIC) {

            cmsCIEXYZ *BlackPtr, BlackXYZ, UntrustedBlackPoint, TrustedBlackPoint, MediaWhite;
            cmsCIELab Lab;

            // If black point is specified, then use it,

            BlackPtr = cmsReadTag(hProfile, cmsSigMediaBlackPointTag);
            if (BlackPtr != NULL) {

                BlackXYZ = *BlackPtr;
                _cmsReadMediaWhitePoint(&MediaWhite, hProfile);

                // Black point is absolute XYZ, so adapt to D50 to get PCS value
                cmsAdaptToIlluminant(&UntrustedBlackPoint, &MediaWhite, cmsD50_XYZ(), &BlackXYZ);

                // Force a=b=0 to get rid of any chroma
                cmsXYZ2Lab(NULL, &Lab, &UntrustedBlackPoint);
                Lab.a = Lab.b = 0;
                if (Lab.L > 50) Lab.L = 50; // Clip to L* <= 50
                cmsLab2XYZ(NULL, &TrustedBlackPoint, &Lab);

                if (BlackPoint != NULL)
                    *BlackPoint = TrustedBlackPoint;

                return TRUE;
            }
    }
#endif

    // That is about v2 profiles.

    // If output profile, discount ink-limiting and that's all
    if (Intent == INTENT_RELATIVE_COLORIMETRIC &&
        (cmsGetDeviceClass(hProfile) == cmsSigOutputClass) &&
        (cmsGetColorSpace(hProfile)  == cmsSigCmykData))
        return BlackPointUsingPerceptualBlack(BlackPoint, hProfile);

    // Nope, compute BP using current intent.
    return BlackPointAsDarkerColorant(hProfile, Intent, BlackPoint, dwFlags);
}