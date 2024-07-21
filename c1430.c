void CMSEXPORT cmsSetHeaderModel(cmsHPROFILE hProfile, cmsUInt32Number model)
{
    _cmsICCPROFILE*  Icc = (_cmsICCPROFILE*) hProfile;
    Icc -> model = (cmsUInt32Number) model;
}