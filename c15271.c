QUtil::safe_fopen(char const* filename, char const* mode)
{
    FILE* f = 0;
#ifdef _WIN32
    // Convert the utf-8 encoded filename argument to wchar_t*. First,
    // convert to utf16, then to wchar_t*. Note that u16 will start
    // with the UTF16 marker, which we skip.
    std::string u16 = utf8_to_utf16(filename);
    size_t len = u16.length();
    size_t wlen = (len / 2) - 1;
    PointerHolder<wchar_t> wfilenamep(true, new wchar_t[wlen + 1]);
    wchar_t* wfilename = wfilenamep.getPointer();
    wfilename[wlen] = 0;
    for (unsigned int i = 2; i < len; i += 2)
    {
        wfilename[(i/2) - 1] =
            static_cast<wchar_t>(
                (static_cast<unsigned char>(u16.at(i)) << 8) +
                static_cast<unsigned char>(u16.at(i+1)));
    }
    PointerHolder<wchar_t> wmodep(true, new wchar_t[strlen(mode) + 1]);
    wchar_t* wmode = wmodep.getPointer();
    wmode[strlen(mode)] = 0;
    for (size_t i = 0; i < strlen(mode); ++i)
    {
        wmode[i] = mode[i];
    }

#ifdef _MSC_VER
    errno_t err = _wfopen_s(&f, wfilename, wmode);
    if (err != 0)
    {
        errno = err;
    }
#else
    f = _wfopen(wfilename, wmode);
#endif
    if (f == 0)
    {
        throw_system_error(std::string("open ") + filename);
    }
#else
    f = fopen_wrapper(std::string("open ") + filename, fopen(filename, mode));
#endif
    return f;
}