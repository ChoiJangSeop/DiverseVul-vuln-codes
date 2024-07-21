int WriteNamedColorCRD(cmsIOHANDLER* m, cmsHPROFILE hNamedColor, cmsUInt32Number Intent, cmsUInt32Number dwFlags)
{
    cmsHTRANSFORM xform;
    cmsUInt32Number i, nColors, nColorant;
    cmsUInt32Number OutputFormat;
    char ColorName[cmsMAX_PATH];
    char Colorant[128];
    cmsNAMEDCOLORLIST* NamedColorList;


    OutputFormat = cmsFormatterForColorspaceOfProfile(hNamedColor, 2, FALSE);
    nColorant    = T_CHANNELS(OutputFormat);


    xform = cmsCreateTransform(hNamedColor, TYPE_NAMED_COLOR_INDEX, NULL, OutputFormat, Intent, dwFlags);
    if (xform == NULL) return 0;


    NamedColorList = cmsGetNamedColorList(xform);
    if (NamedColorList == NULL) return 0;

    _cmsIOPrintf(m, "<<\n");
    _cmsIOPrintf(m, "(colorlistcomment) (%s) \n", "Named profile");
    _cmsIOPrintf(m, "(Prefix) [ (Pantone ) (PANTONE ) ]\n");
    _cmsIOPrintf(m, "(Suffix) [ ( CV) ( CVC) ( C) ]\n");

    nColors   = cmsNamedColorCount(NamedColorList);

    for (i=0; i < nColors; i++) {

        cmsUInt16Number In[1];
        cmsUInt16Number Out[cmsMAXCHANNELS];

        In[0] = (cmsUInt16Number) i;

        if (!cmsNamedColorInfo(NamedColorList, i, ColorName, NULL, NULL, NULL, NULL))
                continue;

        cmsDoTransform(xform, In, Out, 1);
        BuildColorantList(Colorant, nColorant, Out);
        _cmsIOPrintf(m, "  (%s) [ %s ]\n", ColorName, Colorant);
    }

    _cmsIOPrintf(m, "   >>");

    if (!(dwFlags & cmsFLAGS_NODEFAULTRESOURCEDEF)) {

    _cmsIOPrintf(m, " /Current exch /HPSpotTable defineresource pop\n");
    }

    cmsDeleteTransform(xform);
    return 1;
}