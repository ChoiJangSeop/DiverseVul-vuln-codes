cmsUInt32Number CMSEXPORT cmsGetHeaderModel(cmsHPROFILE hProfile)
{
    _cmsICCPROFILE*  Icc = (_cmsICCPROFILE*) hProfile;
    return (cmsUInt32Number) Icc ->model;
}