QUtil::get_current_time()
{
#ifdef _WIN32
    // The procedure to get local time at this resolution comes from
    // the Microsoft documentation.  It says to convert a SYSTEMTIME
    // to a FILETIME, and to copy the FILETIME to a ULARGE_INTEGER.
    // The resulting number is the number of 100-nanosecond intervals
    // between January 1, 1601 and now.  POSIX threads wants a time
    // based on January 1, 1970, so we adjust by subtracting the
    // number of seconds in that time period from the result we get
    // here.
    SYSTEMTIME sysnow;
    GetSystemTime(&sysnow);
    FILETIME filenow;
    SystemTimeToFileTime(&sysnow, &filenow);
    ULARGE_INTEGER uinow;
    uinow.LowPart = filenow.dwLowDateTime;
    uinow.HighPart = filenow.dwHighDateTime;
    ULONGLONG now = uinow.QuadPart;
    return ((now / 10000000LL) - 11644473600LL);
#else
    return time(0);
#endif
}