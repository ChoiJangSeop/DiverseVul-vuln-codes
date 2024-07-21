transcode_utf8(std::string const& utf8_val, std::string& result,
               encoding_e encoding, char unknown)
{
    bool okay = true;
    result.clear();
    if (encoding == e_utf16)
    {
        result += "\xfe\xff";
    }
    size_t len = utf8_val.length();
    for (size_t i = 0; i < len; ++i)
    {
        bool error = false;
        unsigned long codepoint = get_next_utf8_codepoint(utf8_val, i, error);
        if (error)
        {
            okay = false;
            if (encoding == e_utf16)
            {
                result += "\xff\xfd";
            }
            else
            {
                result.append(1, unknown);
            }
        }
        else if (codepoint < 128)
        {
            char ch = static_cast<char>(codepoint);
            if (encoding == e_utf16)
            {
                result += QUtil::toUTF16(ch);
            }
            else
            {
                result.append(1, ch);
            }
        }
        else if (encoding == e_utf16)
        {
            result += QUtil::toUTF16(codepoint);
        }
        else if ((codepoint > 160) && (codepoint < 256) &&
                 ((encoding == e_winansi) || (encoding == e_pdfdoc)))
        {
            result.append(1, static_cast<unsigned char>(codepoint & 0xff));
        }
        else
        {
            unsigned char ch = '\0';
            if (encoding == e_winansi)
            {
                ch = encode_winansi(codepoint);
            }
            else if (encoding == e_macroman)
            {
                ch = encode_macroman(codepoint);
            }
            else if (encoding == e_pdfdoc)
            {
                ch = encode_pdfdoc(codepoint);
            }
            if (ch == '\0')
            {
                okay = false;
                ch = static_cast<unsigned char>(unknown);
            }
            result.append(1, ch);
        }
    }
    return okay;
}