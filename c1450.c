cmsBool BlackPointUsingPerceptualBlack(cmsCIEXYZ* BlackPoint, cmsHPROFILE hProfile)

{
    cmsHTRANSFORM hRoundTrip;
    cmsCIELab LabIn, LabOut;
    cmsCIEXYZ  BlackXYZ;

     // Is the intent supported by the profile?
    if (!cmsIsIntentSupported(hProfile, INTENT_PERCEPTUAL, LCMS_USED_AS_INPUT)) {

        BlackPoint -> X = BlackPoint ->Y = BlackPoint -> Z = 0.0;
        return TRUE;
    }

    hRoundTrip = CreateRoundtripXForm(hProfile, INTENT_PERCEPTUAL);
    if (hRoundTrip == NULL) {
        BlackPoint -> X = BlackPoint ->Y = BlackPoint -> Z = 0.0;
        return FALSE;
    }

    LabIn.L = LabIn.a = LabIn.b = 0;
    cmsDoTransform(hRoundTrip, &LabIn, &LabOut, 1);

    // Clip Lab to reasonable limits
    if (LabOut.L > 50) LabOut.L = 50;
    LabOut.a = LabOut.b = 0;

    cmsDeleteTransform(hRoundTrip);

    // Convert it to XYZ
    cmsLab2XYZ(NULL, &BlackXYZ, &LabOut);

    if (BlackPoint != NULL)
        *BlackPoint = BlackXYZ;

    return TRUE;
}