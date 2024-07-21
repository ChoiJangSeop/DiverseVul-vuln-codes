cmsBool CMSEXPORT cmsDetectDestinationBlackPoint(cmsCIEXYZ* BlackPoint, cmsHPROFILE hProfile, cmsUInt32Number Intent, cmsUInt32Number dwFlags)
{
    cmsColorSpaceSignature ColorSpace;
    cmsHTRANSFORM hRoundTrip = NULL;
    cmsCIELab InitialLab, destLab, Lab;

    cmsFloat64Number MinL, MaxL;
    cmsBool NearlyStraightMidRange = FALSE;
    cmsFloat64Number L;
    cmsFloat64Number x[101], y[101];
    cmsFloat64Number lo, hi, NonMonoMin;
    int n, l, i, NonMonoIndx;


    // Make sure intent is adequate
    if (Intent != INTENT_PERCEPTUAL &&
        Intent != INTENT_RELATIVE_COLORIMETRIC &&
		Intent != INTENT_SATURATION) {
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


    // Check if the profile is lut based and gray, rgb or cmyk (7.2 in Adobe's document)
    ColorSpace = cmsGetColorSpace(hProfile);
    if (!cmsIsCLUT(hProfile, Intent, LCMS_USED_AS_OUTPUT ) ||
        (ColorSpace != cmsSigGrayData &&
         ColorSpace != cmsSigRgbData  &&
         ColorSpace != cmsSigCmykData)) {

        // In this case, handle as input case
        return cmsDetectBlackPoint(BlackPoint, hProfile, Intent, dwFlags);
    }

    // It is one of the valid cases!, presto chargo hocus pocus, go for the Adobe magic

    // Step 1
    // ======

    // Set a first guess, that should work on good profiles.
    if (Intent == INTENT_RELATIVE_COLORIMETRIC) {

        cmsCIEXYZ IniXYZ;

        // calculate initial Lab as source black point
        if (!cmsDetectBlackPoint(&IniXYZ, hProfile, Intent, dwFlags)) {
            return FALSE;
        }

        // convert the XYZ to lab
        cmsXYZ2Lab(NULL, &InitialLab, &IniXYZ);

    } else {

        // set the initial Lab to zero, that should be the black point for perceptual and saturation
        InitialLab.L = 0;
        InitialLab.a = 0;
        InitialLab.b = 0;
    }


    // Step 2
    // ======

    // Create a roundtrip. Define a Transform BT for all x in L*a*b*
    hRoundTrip = CreateRoundtripXForm(hProfile, Intent);
    if (hRoundTrip == NULL)  return FALSE;

    // Calculate Min L*
    Lab = InitialLab;
    Lab.L = 0;
    cmsDoTransform(hRoundTrip, &Lab, &destLab, 1);
    MinL = destLab.L;

    // Calculate Max L*
    Lab = InitialLab;
    Lab.L = 100;
    cmsDoTransform(hRoundTrip, &Lab, &destLab, 1);
    MaxL = destLab.L;

    // Step 3
    // ======

    // check if quadratic estimation needs to be done.
    if (Intent == INTENT_RELATIVE_COLORIMETRIC) {

        // Conceptually, this code tests how close the source l and converted L are to one another in the mid-range
        // of the values. If the converted ramp of L values is close enough to a straight line y=x, then InitialLab
        // is good enough to be the DestinationBlackPoint,
        NearlyStraightMidRange = TRUE;

        for (l=0; l <= 100; l++) {

            Lab.L = l;
            Lab.a = InitialLab.a;
            Lab.b = InitialLab.b;

            cmsDoTransform(hRoundTrip, &Lab, &destLab, 1);

            L = destLab.L;

            // Check the mid range in 20% after MinL
            if (L > (MinL + 0.2 * (MaxL - MinL))) {

                // Is close enough?
                if (fabs(L - l) > 4.0) {

                    // Too far away, profile is buggy!
                    NearlyStraightMidRange = FALSE;
                    break;
                }
            }
        }
    }
    else {
        // Check is always performed for perceptual and saturation intents
        NearlyStraightMidRange = FALSE;
    }


    // If no furter checking is needed, we are done
    if (NearlyStraightMidRange) {

        cmsLab2XYZ(NULL, BlackPoint, &InitialLab);
        cmsDeleteTransform(hRoundTrip);
        return TRUE;
    }

    // The round-trip curve normally looks like a nearly constant section at the black point,
    // with a corner and a nearly straight line to the white point.

    // STEP 4
    // =======

    // find the black point using the least squares error quadratic curve fitting

    if (Intent == INTENT_RELATIVE_COLORIMETRIC) {
        lo = 0.1;
        hi = 0.5;
    }
    else {

        // Perceptual and saturation
        lo = 0.03;
        hi = 0.25;
    }

    // Capture points for the fitting.
    n = 0;
    for (l=0; l <= 100; l++) {

        cmsFloat64Number ff;

        Lab.L = (cmsFloat64Number) l;
        Lab.a = InitialLab.a;
        Lab.b = InitialLab.b;

        cmsDoTransform(hRoundTrip, &Lab, &destLab, 1);

        ff = (destLab.L - MinL)/(MaxL - MinL);

        if (ff >= lo && ff < hi) {

            x[n] = Lab.L;
            y[n] = ff;
            n++;
        }

    }

	// This part is not on the Adobe paper, but I found is necessary for getting any result.

	if (IsMonotonic(n, y)) {

		// Monotonic means lower point is stil valid
        cmsLab2XYZ(NULL, BlackPoint, &InitialLab);
        cmsDeleteTransform(hRoundTrip);
        return TRUE;
	}

    // No suitable points, regret and use safer algorithm
    if (n == 0) {
        cmsDeleteTransform(hRoundTrip);
        return cmsDetectBlackPoint(BlackPoint, hProfile, Intent, dwFlags);
    }


	NonMonoMin = 100;
	NonMonoIndx = 0;
	for (i=0; i < n; i++) {

		if (y[i] < NonMonoMin) {
			NonMonoIndx = i;
			NonMonoMin = y[i];
		}
	}

	Lab.L = x[NonMonoIndx];

    // fit and get the vertex of quadratic curve
    Lab.L = RootOfLeastSquaresFitQuadraticCurve(n, x, y);

    if (Lab.L < 0.0 || Lab.L > 50.0) { // clip to zero L* if the vertex is negative
        Lab.L = 0;
    }

    Lab.a = InitialLab.a;
    Lab.b = InitialLab.b;

    cmsLab2XYZ(NULL, BlackPoint, &Lab);

    cmsDeleteTransform(hRoundTrip);
    return TRUE;
}