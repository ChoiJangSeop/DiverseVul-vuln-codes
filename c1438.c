cmsFloat64Number CMSEXPORT cmsIT8GetDataRowColDbl(cmsHANDLE hIT8, int row, int col)
{
    const char* Buffer;

    Buffer = cmsIT8GetDataRowCol(hIT8, row, col);

    return ParseFloatNumber(Buffer);
}