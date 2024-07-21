unsigned long get_next_utf8_codepoint(
    std::string const& utf8_val, size_t& pos, bool& error)
{
    size_t len = utf8_val.length();
    unsigned char ch = static_cast<unsigned char>(utf8_val.at(pos));
    error = false;
    if (ch < 128)
    {
        return static_cast<unsigned long>(ch);
    }

    size_t bytes_needed = 0;
    unsigned bit_check = 0x40;
    unsigned char to_clear = 0x80;
    while (ch & bit_check)
    {
        ++bytes_needed;
        to_clear |= bit_check;
        bit_check >>= 1;
    }
    if (((bytes_needed > 5) || (bytes_needed < 1)) ||
        ((pos + bytes_needed) >= len))
    {
        error = true;
        return 0xfffd;
    }

    unsigned long codepoint = (ch & ~to_clear);
    while (bytes_needed > 0)
    {
        --bytes_needed;
        ch = utf8_val.at(++pos);
        if ((ch & 0xc0) != 0x80)
        {
            --pos;
            codepoint = 0xfffd;
            break;
        }
        codepoint <<= 6;
        codepoint += (ch & 0x3f);
    }
    return codepoint;
}