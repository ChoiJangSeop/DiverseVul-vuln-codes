QUtil::utf16_to_utf8(std::string const& val)
{
    std::string result;
    // This code uses unsigned long and unsigned short to hold
    // codepoint values. It requires unsigned long to be at least
    // 32 bits and unsigned short to be at least 16 bits, but it
    // will work fine if they are larger.
    unsigned long codepoint = 0L;
    size_t len = val.length();
    size_t start = 0;
    if (is_utf16(val))
    {
        start += 2;
    }
    // If the string has an odd number of bytes, the last byte is
    // ignored.
    for (unsigned int i = start; i < len; i += 2)
    {
        // Convert from UTF16-BE.  If we get a malformed
        // codepoint, this code will generate incorrect output
        // without giving a warning.  Specifically, a high
        // codepoint not followed by a low codepoint will be
        // discarded, and a low codepoint not preceded by a high
        // codepoint will just get its low 10 bits output.
        unsigned short bits =
            (static_cast<unsigned char>(val.at(i)) << 8) +
            static_cast<unsigned char>(val.at(i+1));
        if ((bits & 0xFC00) == 0xD800)
        {
            codepoint = 0x10000 + ((bits & 0x3FF) << 10);
            continue;
        }
        else if ((bits & 0xFC00) == 0xDC00)
        {
            if (codepoint != 0)
            {
                QTC::TC("qpdf", "QUtil non-trivial UTF-16");
            }
            codepoint += bits & 0x3FF;
        }
        else
        {
            codepoint = bits;
        }

        result += QUtil::toUTF8(codepoint);
        codepoint = 0;
    }
    return result;
}