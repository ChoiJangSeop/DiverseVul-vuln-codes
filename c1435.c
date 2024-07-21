cmsFloat64Number CMSEXPORT cmsIT8GetPropertyDbl(cmsHANDLE hIT8, const char* cProp)
{
    const char *v = cmsIT8GetProperty(hIT8, cProp);

    return ParseFloatNumber(v);
}