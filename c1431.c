void CMSEXPORT cmsSetHeaderManufacturer(cmsHPROFILE hProfile, cmsUInt32Number manufacturer)
{
    _cmsICCPROFILE*  Icc = (_cmsICCPROFILE*) hProfile;
    Icc -> manufacturer = (cmsUInt32Number) manufacturer;
}