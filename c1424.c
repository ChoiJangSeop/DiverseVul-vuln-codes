cmsHPROFILE CMSEXPORT cmsCreateBCHSWabstractProfileTHR(cmsContext ContextID,
                                                     int nLUTPoints,
                                                     cmsFloat64Number Bright,
                                                     cmsFloat64Number Contrast,
                                                     cmsFloat64Number Hue,
                                                     cmsFloat64Number Saturation,
                                                     int TempSrc,
                                                     int TempDest)
{
     cmsHPROFILE hICC;
     cmsPipeline* Pipeline;
     BCHSWADJUSTS bchsw;
     cmsCIExyY WhitePnt;
     cmsStage* CLUT;
     cmsUInt32Number Dimensions[MAX_INPUT_DIMENSIONS];
     int i;


     bchsw.Brightness = Bright;
     bchsw.Contrast   = Contrast;
     bchsw.Hue        = Hue;
     bchsw.Saturation = Saturation;

     cmsWhitePointFromTemp(&WhitePnt, TempSrc );
     cmsxyY2XYZ(&bchsw.WPsrc, &WhitePnt);

     cmsWhitePointFromTemp(&WhitePnt, TempDest);
     cmsxyY2XYZ(&bchsw.WPdest, &WhitePnt);

      hICC = cmsCreateProfilePlaceholder(ContextID);
       if (!hICC)                          // can't allocate
            return NULL;


       cmsSetDeviceClass(hICC,      cmsSigAbstractClass);
       cmsSetColorSpace(hICC,       cmsSigLabData);
       cmsSetPCS(hICC,              cmsSigLabData);

       cmsSetHeaderRenderingIntent(hICC,  INTENT_PERCEPTUAL);


       // Creates a Pipeline with 3D grid only
       Pipeline = cmsPipelineAlloc(ContextID, 3, 3);
       if (Pipeline == NULL) {
           cmsCloseProfile(hICC);
           return NULL;
           }

       for (i=0; i < MAX_INPUT_DIMENSIONS; i++) Dimensions[i] = nLUTPoints;
       CLUT = cmsStageAllocCLut16bitGranular(ContextID, Dimensions, 3, 3, NULL);
       if (CLUT == NULL) return NULL;


       if (!cmsStageSampleCLut16bit(CLUT, bchswSampler, (void*) &bchsw, 0)) {

                // Shouldn't reach here
                cmsPipelineFree(Pipeline);
                cmsCloseProfile(hICC);
                return NULL;
       }

       cmsPipelineInsertStage(Pipeline, cmsAT_END, CLUT);

       // Create tags

       if (!SetTextTags(hICC, L"BCHS built-in")) return NULL;

       cmsWriteTag(hICC, cmsSigMediaWhitePointTag, (void*) cmsD50_XYZ());

       cmsWriteTag(hICC, cmsSigAToB0Tag, (void*) Pipeline);

       // Pipeline is already on virtual profile
       cmsPipelineFree(Pipeline);

       // Ok, done
       return hICC;
}