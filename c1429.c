cmsUInt32Number CMSEXPORT cmsGetHeaderManufacturer(cmsHPROFILE hProfile)
{
    _cmsICCPROFILE*  Icc = (_cmsICCPROFILE*) hProfile;
    return (cmsUInt32Number) Icc ->manufacturer;
}