cmsPipeline* _cmsCreateGamutCheckPipeline(cmsContext ContextID,
										  cmsHPROFILE hProfiles[],
										  cmsBool  BPC[],
										  cmsUInt32Number Intents[],
										  cmsFloat64Number AdaptationStates[],
										  cmsUInt32Number nGamutPCSposition,
										  cmsHPROFILE hGamut)
{
	cmsHPROFILE hLab;
	cmsPipeline* Gamut;
	cmsStage* CLUT;
	cmsUInt32Number dwFormat;
	GAMUTCHAIN Chain;
	int nChannels, nGridpoints;
	cmsColorSpaceSignature ColorSpace;
	cmsUInt32Number i;
	cmsHPROFILE ProfileList[256];
	cmsBool     BPCList[256];
	cmsFloat64Number AdaptationList[256];
	cmsUInt32Number IntentList[256];

	memset(&Chain, 0, sizeof(GAMUTCHAIN));


	if (nGamutPCSposition <= 0 || nGamutPCSposition > 255) {
		cmsSignalError(ContextID, cmsERROR_RANGE, "Wrong position of PCS. 1..255 expected, %d found.", nGamutPCSposition);
		return NULL;
	}

	hLab = cmsCreateLab4ProfileTHR(ContextID, NULL);
	if (hLab == NULL) return NULL;


	// The figure of merit. On matrix-shaper profiles, should be almost zero as
	// the conversion is pretty exact. On LUT based profiles, different resolutions
	// of input and output CLUT may result in differences.

	if (cmsIsMatrixShaper(hGamut)) {

		Chain.Thereshold = 1.0;
	}
	else {
		Chain.Thereshold = ERR_THERESHOLD;
	}


	// Create a copy of parameters
	for (i=0; i < nGamutPCSposition; i++) {
		ProfileList[i]    = hProfiles[i];
		BPCList[i]        = BPC[i];
		AdaptationList[i] = AdaptationStates[i];
		IntentList[i]     = Intents[i];
	}

	// Fill Lab identity
	ProfileList[nGamutPCSposition] = hLab;
	BPCList[nGamutPCSposition] = 0;
	AdaptationList[nGamutPCSposition] = 1.0;
	IntentList[nGamutPCSposition] = INTENT_RELATIVE_COLORIMETRIC;


	ColorSpace  = cmsGetColorSpace(hGamut);

	nChannels   = cmsChannelsOf(ColorSpace);
	nGridpoints = _cmsReasonableGridpointsByColorspace(ColorSpace, cmsFLAGS_HIGHRESPRECALC);
	dwFormat    = (CHANNELS_SH(nChannels)|BYTES_SH(2));

	// 16 bits to Lab double
	Chain.hInput = cmsCreateExtendedTransform(ContextID,
		nGamutPCSposition + 1,
		ProfileList,
		BPCList,
		IntentList,
		AdaptationList,
		NULL, 0,
		dwFormat, TYPE_Lab_DBL,
		cmsFLAGS_NOCACHE);


	// Does create the forward step. Lab double to device
	dwFormat    = (CHANNELS_SH(nChannels)|BYTES_SH(2));
	Chain.hForward = cmsCreateTransformTHR(ContextID,
		hLab, TYPE_Lab_DBL,
		hGamut, dwFormat,
		INTENT_RELATIVE_COLORIMETRIC,
		cmsFLAGS_NOCACHE);

	// Does create the backwards step
	Chain.hReverse = cmsCreateTransformTHR(ContextID, hGamut, dwFormat,
		hLab, TYPE_Lab_DBL,
		INTENT_RELATIVE_COLORIMETRIC,
		cmsFLAGS_NOCACHE);


	// All ok?
	if (Chain.hInput && Chain.hForward && Chain.hReverse) {

		// Go on, try to compute gamut LUT from PCS. This consist on a single channel containing
		// dE when doing a transform back and forth on the colorimetric intent.

		Gamut = cmsPipelineAlloc(ContextID, 3, 1);

		if (Gamut != NULL) {

			CLUT = cmsStageAllocCLut16bit(ContextID, nGridpoints, nChannels, 1, NULL);
			cmsPipelineInsertStage(Gamut, cmsAT_BEGIN, CLUT);

			cmsStageSampleCLut16bit(CLUT, GamutSampler, (void*) &Chain, 0);
		}
	}
	else
		Gamut = NULL;   // Didn't work...

	// Free all needed stuff.
	if (Chain.hInput)   cmsDeleteTransform(Chain.hInput);
	if (Chain.hForward) cmsDeleteTransform(Chain.hForward);
	if (Chain.hReverse) cmsDeleteTransform(Chain.hReverse);
	if (hLab) cmsCloseProfile(hLab);

	// And return computed hull
	return Gamut;
}